#include <iostream>
#include <algorithm>
#include <numeric>
#include <valarray>
#include <iterator>
#include <string>
#include <map>
#include <iomanip>
#include "point.hpp"
#include "jarvisMarch.hpp"
#include "bitmap.h"

/*
 * Friend read stream operator
 * First makes a fresh copy, then swaps at the end
 */
istream& operator>>(istream& in, Bitmap& bitmap) {
    // Always start with a fresh copy
    Bitmap b;

    // We want to read from in and place it in the Bitmap
    //in.read(b.header.ftype, sizeof(b.header)-2); // See header for explanation
    in.read(reinterpret_cast<char*>(&b.header), sizeof(b.header));
    if(string(b.header.ftype).compare("BM") < 0 )
        throw BadFileTypeException();
    // Check for first 2 bytes are BM, or else not BMP file

    in.read(reinterpret_cast<char*>(&b.dibs), sizeof(b.dibs));
    // Bitmap File Header

    // Compression 0 means bitdepth will be 24
    // Compression 3 means bitdepth will be 32

    // If 24 bit ( cmpsn 0 ) then we don't get colorspace, and comes in predetrmined
    // order
    if( b.dibs.cmpsn == 3 ){
        in.read(reinterpret_cast<char*>(&b.colorspace), sizeof(b.colorspace) );
        b.setmask( );
    }else{
        // Default values for 24 bpp
        b.r_mask = 2;
        b.g_mask = 1;
        b.b_mask = 0;
    }

    // We have enough information to set aside the memory needed to load the rest
    // of the image

    b._bpp = b.dibs.cDepth>>3;
    b._rowSize = b.__rowSize( b._bpp, b.dibs.width);
    b._rowWidth = b.__rowWidth(b.dibs.cDepth, b.dibs.width );

    b._bits.resize(b.dibs.rawSize);

    in.read(reinterpret_cast<char*>(b._bits.data()), b.dibs.rawSize);
    // If nothings gone wrong, swap it out
    swap(bitmap, move(b));

    return in;
}

/*
 * Friend write stream operator
 */
ostream& operator<<(ostream& out, const Bitmap& b) {
    // Write header
    //out.write( b.header.ftype, sizeof(b.header)-2); // See header for explanation
    out.write(reinterpret_cast<const char*>(&b.header), sizeof(b.header));
    out.write( reinterpret_cast<const char*>(&b.dibs), sizeof(b.dibs));
    // Colorspace if 32bit which is implied by cmpsn != 0
    if( b.dibs.cmpsn )
        out.write( reinterpret_cast<const char*>(&b.colorspace), sizeof(b.colorspace));

    // Then body
    out.write( reinterpret_cast<const char*>(b._bits.data()), b._bits.size());
    return out;
}

/*
 * Friend swap, uses move semantics on the right
 * To keep most operations exception safe we use a copy and swap a lot but the default
 * implementation causes needless copy constructor calls.
 */
void swap( Bitmap &lhs, Bitmap &&rhs ){
    lhs.header      = move(rhs.header);
    lhs.dibs        = move(rhs.dibs);
    lhs.colorspace  = move(rhs.colorspace);
    lhs.r_mask      = move(rhs.r_mask);
    lhs.g_mask      = move(rhs.g_mask);
    lhs.b_mask      = move(rhs.b_mask);
    lhs.a_mask      = move(rhs.a_mask);
    lhs._rowSize     = move(rhs._rowSize);
    lhs._rowWidth    = move(rhs._rowWidth);
    lhs._bpp         = move(rhs._bpp);
    lhs._bits        = move(rhs._bits);
}

/*
 * Copy constructor
 * @ param noData - if true instructs the copy constructor to not copy the data portion, in this case
 * it will size the vector but not copy the data.
 */
Bitmap::Bitmap( const Bitmap &rhs, bool noData ): 
header{rhs.header},
dibs{rhs.dibs},
colorspace{rhs.colorspace},
r_mask{rhs.r_mask},
g_mask{rhs.g_mask},
b_mask{rhs.b_mask},
a_mask{rhs.a_mask},
_rowSize{rhs._rowSize},
_rowWidth{rhs._rowWidth},
_bpp{rhs._bpp},
_bits{}
{
    if( noData ){
        _bits.resize(rhs._bits.size()); // Set the size only
    }else{
        _bits = rhs._bits;
    }
}

/*
Internal use only, takes the mask order and determines which order the masks are in
and sets the internel masks for position within a pixel. This is only called in case
of 32 bit and is not needed for 24 bit as those masks are set in the standard.

Throws BadMaskOrderException if the input is invalid.
*/
void Bitmap::setmask(){
    string masks = string(reinterpret_cast<char *>(&this->colorspace.masko), sizeof(this->colorspace.masko));
    int size = masks.size();
    for( int i = 0; i < size; ++i ){
        switch(masks[i]){
            case 'B':
                b_mask = maskToInt( colorspace.mask[i] );
                break;
            case 'G':
                g_mask = maskToInt( colorspace.mask[i] );
                break;
            case 'R':
                r_mask = maskToInt( colorspace.mask[i] );
                break;
            case 's':
                a_mask = maskToInt( colorspace.mask[i] );
                break;
            default:
                throw BadMaskOrderException();
                break;
        }
    }
}

/*
 * Internal use only, finds the integer position of which byte the mask is for
 * @param a mask such as 0x00FF0000, with FF and 00 always coming in pairs.
 * @return the position from the left so that if read as a character array that would be the position.
 */
uint32_t Bitmap::maskToInt( uint32_t mask ) noexcept{
    // If we are assuming only the four positions we could make this into a switch statement instead
    uint32_t val = 0;
    // Take one off the top, either this or minus one from the return
    mask = mask >> 8;
    while( mask ){
        val++;
        mask = mask >> 8;
    }
    return val;
}

/*
 * Retrieves the single pixel/color
 * @param x is the x coordinate
 * @param y is the y coordinate
 * @param mask is the integer position mask, not to be confused with an actual bitmask
 * Does not check bounds!
 */
inline uint8_t& Bitmap::getPixel( int x, int y, uint32_t mask ){
    return const_cast<uint8_t&>(static_cast<const Bitmap&>(*this).getPixel(x,y,mask));
}

inline const uint8_t& Bitmap::getPixel(int x, int y, uint32_t mask) const{
    if( x > dibs.width || y > ( dibs.height < 0 ? -dibs.height : dibs.height ) )
        throw OutOfBoundsException();
    if( dibs.height < 0 )
        y = (-dibs.height) - y;
    return _bits[ y*_rowWidth + (x*_bpp) + mask ];
}
/*
 * Takes a value and returns either the low or high depending on whether it is above the threshold or not [0x00,0x54)
 */
inline uint8_t clip( uint8_t value )noexcept{
    return (value < 0x40 ? 0x00 : (value < 0xC0 ? 0x80 : 0xFF ) );
}

/*
 * Peforms a cell (sic) shade operation over the entire image
 */
void cellShade(Bitmap& b)noexcept{
    for_each(b.begin(),b.end(),[&b](auto& value){
        *(&value+b.rmask()) = clip(*(&value+b.rmask()) );
        *(&value+b.gmask()) = clip(*(&value+b.gmask()) );
        *(&value+b.bmask()) = clip(*(&value+b.bmask()) );
    });
}

/*
 * Performs a grayscale operation over entire image
 * In this version I use the numbers given from Wikipedia concerning grayscale luminence ratios
 * which give a more realistic grayscale than averaging.
 */
void grayscale(Bitmap& b ) {
    int count = 0;
    for_each(b.begin(),b.end(),[&b,&count](auto& value){
        ++count;
        uint8_t y = *(&value+b.rmask())*0.216
                  + *(&value+b.gmask())*0.7152
                  + *(&value+b.bmask())*0.0722;
        *(&value+b.rmask()) = y;
        *(&value+b.gmask()) = y;
        *(&value+b.bmask()) = y;
    });
}

/*
 * Performs a gaussian blur operation over entire image
 * Calls a function that could throw, but we always call it with two matrices of the same
 * size which are the conditions that it throws, so meh. We do a copy-swap on the image.
 */
void blur(Bitmap& b ) {
    // Slide 35    
    // This is a more intense piece, we'll need to do an elementwise matrix multiplication
    // We'll want to work on a copy as we'll need the unaffected section
    Bitmap gauss(b);

    // We'll load a vector flattened with the gaussian blurr, and then pull from our original image
    // while pushing to our copy.

    const valarray<uint32_t> matrix= {
        1,  4,  6,  4, 1,
        4, 16, 24, 26, 4,
        6, 24, 36, 24, 6,
        4, 16, 24, 26, 4,
        1,  4,  6,  4, 1
    };
    // We'll use a uint32_t to store the result, then scale it down.
    // We'll have 8bit, multiplied by most a 6bit, needing 14bits, then added together with the most 25 times, so another 5 bits, making
    // a total of 19bits needed. A 32bit int can hold the entire summation, and arguabbly a 16bit int is all we need for the matrix
    // itself
    valarray<uint32_t> result[3];
    for( auto& v: result ){
        v.resize(matrix.size());
    }
    for( int j = 0; j < b.height(); ++j ){
        for( int i = 0; i < b.width(); ++i ){
            int xindex = 2;
            int yindex = -2;
            // Load the matrix
            for( uint32_t k = 0; k < matrix.size(); ++k ){
                // We'll do it a slow way at first, then think about optimization
                // The biggest roadblock to a good algorithm is optimizing too early
                result[0][k] = b.r( clamp(i+xindex, 0, b.width()-1 ), clamp(j+yindex, 0, b.height()-1) );
                result[1][k] = b.g( clamp(i+xindex, 0, b.width()-1 ), clamp(j+yindex, 0, b.height()-1) );
                result[2][k] = b.b( clamp(i+xindex, 0, b.width()-1 ), clamp(j+yindex, 0, b.height()-1) );
                // Update indexes
                if( yindex == 2 ){
                    --xindex;
                    yindex = -2;
                }else{
                    ++yindex;
                }
            } // k
            // hadamard

            for( uint32_t i = 0; i < 3; ++i ){
                result[i] *= matrix;
            }
            // Now stuff it back in
            gauss.r( i,j ) = clamp( ((result[0].sum()) >> 8 ), 0u, 255u) ;
            gauss.g( i,j ) = clamp( ((result[1].sum()) >> 8 ), 0u, 255u) ;
            gauss.b( i,j ) = clamp( ((result[2].sum()) >> 8 ), 0u, 255u) ;
            // Update indexes
        } // j
    } // i
    swap(b,move(gauss));
}

/*
 * Performs a pixalation operation over entire image
 */ 
void pixelate(Bitmap& b) {
    // The idea here is to take a percentage of the width to use as the diameter. If it is less than 100
    // then we'll take the midpoint. We start at a half radius from the edge and move from there.
    Bitmap pix(b);
    valarray<uint32_t> matrix[3];
    uint32_t result[3] = {0,0,0};
    for( auto& v: matrix ){
        v.resize(16*16);
    }
    // Use a marching algorithm instead of loops of loops

    for( int j = 0; j < b.height(); j += 16 ){
        for( int i = 0; i < b.width(); i += 16 ){
            for( int yindex = 0; yindex < 16 && j + yindex < b.height(); ++yindex ){
                for( int xindex = 0; xindex < 16 && i + xindex < b.width(); ++xindex ){
                    matrix[0][(yindex<<4)+xindex] = b.r( i+xindex, j+yindex );
                    matrix[1][(yindex<<4)+xindex] = b.g( i+xindex, j+yindex );
                    matrix[2][(yindex<<4)+xindex] = b.b( i+xindex, j+yindex );
                } // yindex
            } // xindex
            // Get the average into result
            result[0] = matrix[0].size() ? matrix[0].sum()/matrix[0].size() : 0;
            result[1] = matrix[1].size() ? matrix[1].sum()/matrix[1].size() : 0;
            result[2] = matrix[2].size() ? matrix[2].sum()/matrix[2].size() : 0;
            // Now stuff it back in
            for( int yindex = 0; yindex < 16 && j+yindex < b.height() ; ++yindex ){
                for( int xindex = 0; xindex < 16 && i+xindex < b.width() ; ++xindex ){
                    pix.r( i + xindex, j + yindex ) = result[0];
                    pix.g( i + xindex, j + yindex ) = result[1];
                    pix.b( i + xindex, j + yindex ) = result[2];
                }
            }

        } // j
    } // i
    swap(b,move(pix));
}

/*
 * Resizes an images dimensions but does not pad existing image, essentially image is corrupted after this call
 * @param width new width
 * @param height new height
 */
void Bitmap::setDimension( int32_t width, int32_t height ){
    // make a copy of DIBS
    // set new size for bmp
    // set width and height
    // resize internal representation to match padding CORRUPTS DATA!!

    // Copy DIBS
    DIBs _d(dibs);

    // Set height and width
    _d.height = height;
    // Should we throw on negative width?
    if( width <= 0 )
        throw InvalidWidthException();
    _d.width = width;

    // Calculate RowSize
    uint32_t rowWidth   = __rowWidth(_d.cDepth, _d.width);
    uint32_t rowSize    = __rowSize(_bpp, _d.width);

    // Calculate new size
    _d.rawSize = __rawSize(_d.height, rowWidth);

    // Reset internal rpresentation
    if( _bits.size() != _d.rawSize )
        _bits.resize( _d.rawSize );

    // If no exceptions we can now copy everything over
    this->dibs      = _d;
    this->_rowSize   = rowSize;
    this->_rowWidth  = rowWidth;
    this->header.size = _d.rawSize + header.offset;

}

/*
 * Rotates the image clockwise 90*
 */
void rot90(Bitmap& o) {
    Bitmap b(o, true);
    b.setDimension( o.height(), o.width() );

    // Iterate through
    for( int j = 0; j < b.height(); ++j ){
        int x = b.height() - 1 - j;
        for( int i = 0; i < b.width(); ++i ){
            int y = i;
            b.r( i, j ) = o.r( x, y );
            b.g( i, j ) = o.g( x, y );
            b.b( i, j ) = o.b( x, y );
            if( o.hasAlpha() ){
                b.a( i, j ) = o.a( x, y );
            }
        }
    }
    swap(o,move(b));
}

/*
 * Rotates the image both clockwise and counterclockwise 180* :-)
 */
void rot180(Bitmap& o ) {
    // Similar idea, We'll just read through the file rewriting it, but no change in dimension.
    Bitmap b(o, true);

    // Iterate through
    for( int j = 0; j < b.height(); ++j ){
        int y = b.height() - 1 - j;
        for( int i = 0; i < b.width(); ++i ){
            int x = b.width() - 1 - i;
            b.r( i, j ) = o.r( x, y );
            b.g( i, j ) = o.g( x, y );
            b.b( i, j ) = o.b( x, y );
            if( o.hasAlpha() ){
                b.a( i, j ) = o.a( x, y );
            }
        }
    }
    swap(o, move(b));
}

/*
 * Rotates the image clockwise 270*
 */
void rot270(Bitmap& o ) {
    Bitmap b(o, true);
    b.setDimension( o.height(), o.width() );

    // Iterate through
    for( int j = 0; j < b.height(); ++j ){
        int x = j;
        for( int i = 0; i < b.width(); ++i ){
            int y = b.width() -1 -i;
            b.r( i, j ) = o.r( x, y );
            b.g( i, j ) = o.g( x, y );
            b.b( i, j ) = o.b( x, y );
            if( o.hasAlpha() ){
                b.a( i, j ) = o.a( x, y );
            }
        }
    }
    swap(o,move(b));
}

/*
 * Flips the image across the vertical center |
 */
void flipv(Bitmap& b ) {
    // cannot have negative width
    // b.setWidth( b.width() * -1 );
    Bitmap pix(b, true);
    for( int32_t j = 0; j < b.height() ; ++j ){
        for( int32_t i = 0; i < b.width() >> 1; ++i ){
            int32_t i2 = b.width() - 1 - i;
            // Now swap
            // Could consider swapping by pixel instead of color, perhaps an auto pointer for type deduction
            // would allow this easily.
            pix.r( i, j ) = b.r( i2, j );
            pix.g( i, j ) = b.g( i2, j );
            pix.b( i, j ) = b.b( i2, j );
            pix.r( i2, j ) = b.r( i, j );
            pix.g( i2, j ) = b.g( i, j );
            pix.b( i2, j ) = b.b( i, j );
            if( b.hasAlpha() ){
                pix.a( i, j ) = b.a( i2, j );
                pix.a( i2, j ) = b.a( i, j );
            }
        } // j
    } // i
    swap(b, move(pix));
}

/*
 * Flips the image across the center horizontally --
 */
void fliph(Bitmap& b ) noexcept{
    // Cheap and easy way to flip across the horizontal.
     b.setHeight( b.height() * -1 );
}

/*
 * Flips over the first diagonal \
 */
void flipd1(Bitmap& o ) {
    // A little bit of group theory should go a long ways. This should be essentially a transpose
    Bitmap b(o, true);
    b.setDimension( o.height(), o.width() );

    // Iterate through
    for( int j = 0; j < b.height(); ++j ){
        for( int i = 0; i < b.width(); ++i ){
            b.r( i, j ) = o.r( j, i );
            b.g( i, j ) = o.g( j, i );
            b.b( i, j ) = o.b( j, i );
            if( o.hasAlpha() ){
                b.a( i, j ) = o.a( j, i );
            }
        }
    }
    swap(o,move(b));
}

/*
 * Flips over the second diagonal /
 */
void flipd2(Bitmap& o ) {
    // A little bit of group theory should go a long ways. This should be essentially a transpose
    Bitmap b(o, true);
    b.setDimension( o.height(), o.width() );

    // Iterate through
    for( int j = 0; j < b.height(); ++j ){
        int x = o.width() - 1 - j;
        for( int i = 0; i < b.width(); ++i ){
            int y = o.height() - 1 - i;
            b.r( i, j ) = o.r( x, y );
            b.g( i, j ) = o.g( x, y );
            b.b( i, j ) = o.b( x, y );
            if( o.hasAlpha() ){
                b.a( i, j ) = o.a( x, y );
            }
        }
    }
    swap(o,move(b));
}

/*
 * Doubles the size of the image by duplicating rows and columns
 */
void scaleUp(Bitmap& o ) {
    Bitmap b(o, true);

    b.setDimension( b.width() << 1, b.height() << 1 );

    //
    for( int j = 0; j < o.height(); ++j ){
        int y = j << 1;
        for( int i = 0; i < o.width(); ++i ){
            // Shift the bits since we are doubling
            // Blocking operations together
            int x = i << 1;

            uint8_t pixel = o.r( i, j );
            b.r( x, y ) = pixel;
            b.r( x + 1, y ) = pixel;
            b.r( x, y+1 ) = pixel;
            b.r( x + 1, y+1 ) = pixel;
            
            pixel = o.g( i, j );
            b.g( x, y ) = pixel;
            b.g( x + 1, y ) = pixel;
            b.g( x, y+1 ) = pixel;
            b.g( x + 1, y+1 ) = pixel;

            pixel = o.b( i, j );
            b.b( x, y ) = pixel;
            b.b( x + 1, y ) = pixel;
            b.b( x, y+1 ) = pixel;
            b.b( x + 1, y+1 ) = pixel;

            if( o.hasAlpha() ){
                pixel = o.a( i, j );
                b.a( x, y ) = pixel;
                b.a( x + 1, y ) = pixel;
                b.a( x, y+1 ) = pixel;
                b.a( x + 1, y+1 ) = pixel;
            }
        }
    }
    swap(o, move(b));
}
template<int GROUP, typename IN, typename OUT>
void copy_every_n_in_groups_of_m(IN it, IN id, OUT ot, size_t n ){
    for(; it != id; it+=(n*GROUP)){
        for(auto i=0; i< GROUP; ++i){
            *ot = *(it+i);
            ++ot;
        }
    }
}
/*
 * Scales down the image by halving the rows and columns.
 */
void scaleDown(Bitmap& o ) {
    Bitmap b(o, true);

    b.setDimension( b.width() >> 1, b.height() >> 1 );

    vector<decltype (b.getBits().begin())> its;
    for(int j =0; j < o.height(); j+=2 ){
        its.push_back(o.getBits().begin()+(j*o.rowWidth()));
    }
    auto ot = b.getBits().begin();
    for(auto& it: its){
        if(o.bpp() == 4){
            copy_every_n_in_groups_of_m<4> (it,it+(o.rowWidth()),ot,2);
        }else{
            copy_every_n_in_groups_of_m<3> (it,it+(o.rowWidth()),ot,2);
        }
        ot+=b.rowWidth();
    }
    swap( o, move(b) );
}

// Here's what drives our function
void contours(Bitmap&o, int32_t isovalues, int32_t stepsize, bool useBinaryBitmap){
    Bitmap b(o);
    auto cont = findContours(b,isovalues, stepsize, useBinaryBitmap);

    auto process_cont{cont};
//    vector<vector<pt>> jm;
//    for( auto& i: process_cont){
//        jm.push_back(jarvisMarch(i));
//    }
//    for( auto& i: jm){
//        for(auto& j: i){
//            draw( o, j.x, j.y, 0x00FF00, 8);
//        }
//    }

      process_cont = cont;
      vector<vector<pt>> hulls;
      for( auto& i: process_cont ){
          if(i.size() > 3 )
            hulls.push_back(grahamScan(i));
      }
      
      for( auto& i: hulls){
          for(auto& j: i){
              draw( o, j.x, j.y, 0xFF00FF, o.width()/1000 + 8);
          }
      }
      
      //bao trying
      for(auto & hull:hulls){
          if( hull.empty() ) // happens when polygon is colinier
          {
              continue;
          }
	      for(size_t p = 0; p < hull.size()-1; ++p){
		drawLine(o, hull[p], hull[p+1],0xFF22FF,2);
	      }
		drawLine(o, hull.front(), hull.back(),0xFF22FF,2);
      }

    int count = 0;
    uint32_t color = 0xFF0000;
    for( auto& i: cont){
        for(auto& j: i){
            ++count;
            draw( o, j.x, j.y, color, o.width()/1000 + 2);
        }
        color += 0x101123;
    }
    //cout << "Count: " << count << endl;
}

vector<vector<pt > > findContours(const Bitmap& o, int32_t isovalue, uint32_t step, bool useBinaryInterp)
{
    Bitmap b(o);
    binaryGray(b, isovalue);

    const Bitmap &interBitmap = useBinaryInterp ? b : o;
   // Make it a binary by using a threshold and transforming the entire thing

    // The grids are independent of other grids
    transform( b.getBits().begin(), b.getBits().end(), b.getBits().begin(),
        [](auto value){ return (value == 255) ? 1: 0; } );

    // For now, our map of points
    map<pt,pair<edge,edge>,PointEquality<point_t>> points;

    // Some information for moving iterators
    // ensure against negative height
    const uint32_t h = static_cast<uint32_t>(b.height()*(b.height() < 0 ? -1:1));
    const uint32_t w = static_cast<uint32_t>(b.width());
    const uint32_t bpp = b.bpp();
    const uint32_t steps = bpp*step;

    // The four corners march fourth on their horses towards the apocalypse

    auto rb = b.getBits().begin()+b.rmask()+steps;
    auto rt = rb+w*steps + b.padding()*step;

    // Got it all into one statement without conditionals :-D
    uint32_t leap = bpp*(w*(step-1)+w%step + !(w%step)*step)+b.padding()*(step);

    for( uint32_t j = 0; j < h-step; j+=step )
    {
        // Set the bits in 2 and 3 as the next step will move them to the correct 1 and 4 position
        uint8_t ot = *(rt-steps) << 2 | *(rb-steps) << 1;

        for( uint32_t i = 0; i < w-step; i+=step )
        {
            // Bitwise map pos 2,3 to 1,4
            ot = composeBits(ot, *rb, *rt);
            if(ot != 0 && ot != 15){
                // Use *ot to add to pt
                auto vs = edges(ot); // our v's
                auto v = vs.front();
                if( vs.size() > 1 ){
                    // Do something with these because of ambiguous case
                    // Or just ignore it and be consistent in always choosing the first.
                    // Form Isosurfaces (by Rephael Wenger)
                    /*
                     * While the choice of isocontours for the ambiguous configurations change the
                     * isocontour topology, any of the choices will produce isocntours that are
                     * 1-manifolds and strictly separate strictly positive vertices from  negative
                     * vertices.
                     */
                }
                // map edges into square space from unit square space
                // v.first == v, v.second == v'
                // Points is a map from i,j -> edge pairs

                // in here we want to add our edges to S (the set of all edges)
                // We'll do (e1+(i,j)),(e2+(i,j)) as our edge pairs
                // make_edge gurantees vertexs are ordered
                points.insert(make_pair(pt(i,j),make_pair(
                                     make_edge<point_t>(
                                           pt(i,j)+(v.first.first)*step,
                                           pt(i,j)+(v.first.second)*step
                                          ),
                                     make_edge<point_t>(
                                            pt(i,j)+(v.second.first)*step,
                                            pt(i,j)+(v.second.second)*step
                                           )
                                     ))
                                 );
            }
            rt+=steps;
            rb+=steps;
        }
        rt+=leap;
        rb+=leap;
        // If there is padding then we'll need to jump forward here
        // lt+=padding; rt+=padding; lb+=padding; rb+=padding;
    }
    // Now that we have our composed vector we can construct our single set of points to
    // complete the first step

    // Idea here, go through each edge pair finding the one that connects, trace out the
    // shape and remove these items from the vector and restart the process

    // Interpolation, should be a fun use of transform :-D
    // And then it wasn't, for ftw

    map<pt,pair<pt,pt>,PointEquality<point_t>> interpolated_points;

    for(auto i: points){
        pt e1 = interpolation(i.second.first.first, i.second.first.second, interBitmap.r(i.second.first.first), interBitmap.r(i.second.first.second), 0 );
        pt e2 = interpolation(i.second.second.first, i.second.second.second, interBitmap.r(i.second.second.first), interBitmap.r(i.second.second.second), 0 );

        interpolated_points[i.first] = make_pair( e1, e2 );
    }

    vector<vector<pt>> polygons;
    while(!interpolated_points.empty())
    {
        vector<pt> poly = {};

        // Put the point into our polygon
        // 1. Find a point that shares the first edge
        // 2. Place point in bag, remove shared edge, check for next edge
        // 3. ...
        // 4. Profit!!
        // 5. If next edge == current.second, then stop

        // Prime the pump
        // Get the first element by iterator since this is a map
        // For a map it->first == key, it->second == value
        auto it = interpolated_points.begin();
        pt first_pt = it->first;
        poly.emplace_back(first_pt);
        pt last_edge = it->second.second;
        pt current_edge = it->second.first;
        pt cv = first_pt;

        // Remove point
        interpolated_points.erase(it);

        // Then find the next
        bool done = false;
        while( !done ){
            vector<pair<pt,pair<pt,pt>>> found;
            for_each( interpolated_points.begin(), interpolated_points.end(),
                      [&current_edge,&found](auto value){
                // Check both edges
                if (value.second.first == current_edge || value.second.second == current_edge){
                    found.emplace_back(value);
                }
            } );
            if( found.empty()){
                // No points
                //cout << "Broken Segment, Starting new Polygon" << endl;
                break;
            }
            auto best_pt = found.back(); found.pop_back();

            // Find the closest point if there is ambiquity
            // We might need to instead track the previous point and
            // Find which direction we came from to know where to go.
            for( auto i: found ){
                if( distance( cv, i.first) < distance( cv, best_pt.first)){
                    best_pt = i;
                }
            }

            // Update the current vertex
            cv = best_pt.first;

            if( current_edge == best_pt.second.first){
                poly.emplace_back(best_pt.second.first);
                current_edge = best_pt.second.second;
            }else{
                poly.emplace_back(best_pt.second.second);
                current_edge = best_pt.second.first;
            }
            interpolated_points.erase(best_pt.first);

            if( current_edge == last_edge ){
                //cout << "Finished Building Polygon" << endl;
                done = true;
            }
        }
        polygons.emplace_back(poly);
    }

    return polygons;
}

inline uint8_t composeBits(uint8_t b, uint8_t b2, uint8_t b3 ){
    // This maps 2,3 to 1,4, then sets 2, 3 to new bits
    // This allows us to march forward with only two iterators
    return ( (b << 1 | b >> 1) & 0b1001 ) | b2 << 1 | b3 << 2;
}
// sp != sq or else arithmetic error, divide by zero
inline pt interpolation( pt p, pt q, point_t sp, point_t sq, point_t sigma){
    double alpha = (sigma - sp)/(sq - sp);
    return pt( (1-alpha)*p.x + alpha*q.x, (1-alpha)*p.y + alpha*q.y );
}

void draw(Bitmap&o, uint32_t x, uint32_t y, uint32_t color, uint32_t thickness ){
    // Need a good way to pull out the color, or split this, but for now we'll
    // Leave this like this
    for( uint32_t i = 0; i < thickness && x+i < o.width(); ++i){
        for( uint32_t j = 0; j < thickness && y+j < o.height(); ++j ){
            o.r(x+i,y+j) = (color & 0xFF0000) >> 16;
            o.g(x+i,y+j) = (color & 0x00FF00) >> 8;
            o.b(x+i,y+j) = (color & 0x0000FF);
        }
    }
}

void drawLine(Bitmap & o, const pt & p1, const pt & p2, uint32_t color, uint32_t thickness){
	point_t delta_x, delta_y;

	if(p1.x != p2.x){	
		pt lp = p1.x < p2.x? p1:p2;
		pt rp = p1.x < p2.x? p2:p1;

		for(uint32_t i = lp.x; i <= rp.x; ++i){
			delta_x = i - lp.x;
			delta_y = delta_x * (rp.y - lp.y) / (rp.x - lp.x);

			draw(o, lp.x + delta_x, lp.y + delta_y, color, thickness);
		}
	}
	else{
		pt bp = p1.y < p2.y? p1:p2;
		pt tp = p1.y < p2.y? p2:p1;

		for(uint32_t i = bp.y; i <= tp.y; ++i){
			draw(o, bp.x, i, color, thickness);
		}
	}
}

void binaryGray(Bitmap &o, const int32_t isovalue){
    grayscale(o);
    transform(o.getBits().begin(), o.getBits().end(),o.getBits().begin(),
              [&isovalue](auto value){return value > isovalue ? 255 : 0;});
}

/*
 * The idea here is to return a set of a pair of edges. Why this isn't just a pair of
 * edges is due to the ambiguous case where there are two possible pairs of edges.
 * We'll define an edge as two vertices, (v,v')
 *
 * We want to map from the unit square to the square at (i,j), but our table will only
 * return the unit square edges, we'll do the mapping after calling edges.
 */
vector<pair<edge,edge>> edges( uint8_t square ){
    vector<pair<edge,edge>> sides;
    switch( square ){
    case 1:                                             /* ********************/
    case 14:                                            // Bottom, Left       */
        sides = { make_pair( edge( pt(0,0),pt(0,1) ),   // +==+               */
                             edge( pt(0,0),pt(1,0) )    // |  |               */
                           )                            // -==+               */
                };                                      /* ********************/
        break;

    case 2:                                             /* ********************/
    case 13:                                            //  Bottom, Right     */
        sides = { make_pair( edge( pt(0,0),pt(1,0) ),   // +==+               */
                             edge( pt(1,0),pt(1,1) )    // |  |               */
                           )                            // +==-               */
                };                                      /* ********************/
        break;

    case 3:                                             /* ********************/
    case 12:                                            // Left, Right        */
        sides = { make_pair( edge( pt(0,0),pt(0,1) ),   // +==+               */
                             edge( pt(1,0),pt(1,1) )    // |  |               */
                           )                            // -==-               */
                };                                      /* ********************/
        break;

    case 4:                                             /* ********************/
    case 11:                                            // Top, Right         */
        sides = { make_pair( edge( pt(0,1),pt(1,1) ),   // +==-               */
                             edge( pt(1,0),pt(1,1) )    // |  |               */
                           )                            // +==+               */
                };                                      /* ********************/
        break;

    case 5:                                             /* ******************************/
    case 10:                                            // {Top, Right}, {Bottom, Left} */
        sides = { make_pair( edge( pt(0,1),pt(1,1) ),   // -==+                         */
                             edge( pt(1,1),pt(1,0) )    // |  |                         */
                           ),                           // +==-                         */
                  make_pair( edge( pt(0,0),pt(0,1) ),   /* ******************************/
                             edge( pt(0,0),pt(1,0) )
                           )
                };
        break;

    case 6:                                             /* ********************/
    case 9:                                             // {Top, Bottom}      */
        sides = { make_pair( edge( pt(0,1),pt(1,1) ),   // +==-               */
                             edge( pt(0,0),pt(1,0) )    // |  |               */
                           )                            // +==-               */
                };                                      /* ********************/
        break;

    case 7:                                             /* ********************/
    case 8:                                             // {Top, Left}        */
        sides = { make_pair( edge( pt(0,1),pt(1,1) ),   // +==-               */
                             edge( pt(0,0),pt(0,1) )    // |  |               */
                           )                            // -==-               */
                };                                      /* *********************/
        break;
    /* **********************
     * None
     ***********************/
    case 0:
    case 15:
    default:
        break;
    }
    return sides;
}
