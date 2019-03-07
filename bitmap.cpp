#include <iostream>
#include <algorithm>
#include <numeric>
#include <valarray>
#include <iterator>
#include <string>
#include <map>
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

    b.Bpp = b.dibs.cDepth>>3;
    b.rowSize = ( ( b.dibs.cDepth * b.dibs.width + 31 ) / 32 ) * 4;

    b.bits.resize(b.dibs.rawSize);

    in.read(reinterpret_cast<char*>(b.bits.data()), b.dibs.rawSize);
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
    out.write( reinterpret_cast<const char*>(b.bits.data()), b.bits.size());
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
    lhs.rowSize     = move(rhs.rowSize);
    lhs.rowWidth    = move(rhs.rowWidth);
    lhs.Bpp         = move(rhs.Bpp);
    lhs.bits        = move(rhs.bits);
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
rowSize{rhs.rowSize},
rowWidth{rhs.rowWidth},
Bpp{rhs.Bpp},
bits{}
{
    if( noData ){
        bits.resize(rhs.bits.size()); // Set the size only
    }else{
        bits = rhs.bits;
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
    return bits[ y*rowSize + (x*Bpp) + mask ];
}
/*
 * Takes a value and returns either the low or high depending on whether it is above the threshold or not [0x00,0x54)
 */
uint8_t clip( uint8_t value )noexcept{
    return (value < 0x40 ? 0x00 : (value < 0xC0 ? 0x80 : 0xFF ) );
}

/*
 * Peforms a cell (sic) shade operation over the entire image
 */
void cellShade(Bitmap& b)noexcept{
    for( int j = 0; j < b.height(); ++j ){
        for( int i = 0; i < b.width(); ++i ){
            // How to cel shade
            // We'll grab each color and clip them.
            uint8_t &bv = b.b(i,j);
            uint8_t &gv = b.g(i,j);
            uint8_t &rv = b.r(i,j);
            bv = clip( bv );
            gv = clip( gv );
            rv = clip( rv );
        }
    }
}

/*
 * Performs a grayscale operation over entire image
 * In this version I use the numbers given from Wikipedia concerning grayscale luminence ratios
 * which give a more realistic grayscale than averaging.
 */
void grayscale(Bitmap& b ) {
    for( int j = 0; j < b.height(); ++j ){
        for( int i = 0; i < b.width(); ++i ){
            // Now gray scale it using the formula
            // Y = 0.216*R + 0.7152*G + 0.0722*B
            // Hmm, but where to put this
            // Okay, looks like every color gets the same value
            uint8_t &rv = b.r(i,j);
            uint8_t &gv = b.g(i,j);
            uint8_t &bv = b.b(i,j);
            uint8_t y = 0.216*rv + 0.7152*gv + 0.0722*bv; // Wikipedia
            //double y = 0.2989*rv + 0.5870*gv + 0.1140*bv; // Matlab
            rv = y;
            gv = y;
            bv = y;
        }
    }
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
 * Takes a flattened matrix and returns the average
 * return average of all elements
 */
uint32_t matrixAverage( const vector<uint32_t>& rhs )noexcept{
    uint32_t sum = 0;
    for( auto v: rhs ){
        sum += v;
    }
    return rhs.empty() ? sum : sum / rhs.size();
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
    // TODO: to not corrupt we would need to remove padding from middle (not great for vectors)
    // then add it to wherever it should be. Of course, there is no way to know really how the user
    // would want us to handle the setDimension, black in empty spots? Cropping sides that no longer fit
    // Really, should only be used when you will be setting data in, such as when doing a rotation.

    // We'll handle 32bit first then work on 24bit

    // Copy DIBS
    DIBs _d(dibs);

    // Set height and width
    _d.height = height;
    // Should we throw on negative width?
    if( width <= 0 )
        throw InvalidWidthException();
    _d.width = width;

    // Calculate RowSize
    uint32_t _rowSize   = ((_d.cDepth * _d.width + 31 ) >> 5 ) << 2;

    // Calculate new size
    _d.rawSize = _rowSize * ( _d.height < 0 ? -_d.height: _d.height );

    // Reset internal rpresentation
    if( bits.size() != _d.rawSize )
        bits.resize( _d.rawSize );

    // If no exceptions we can now copy everything over
    this->dibs      = _d;
    this->rowSize   = _rowSize;
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

/*
 * Scales down the image by halving the rows and columns.
 */
void scaleDown(Bitmap& o ) {
    Bitmap b(o, true);

    b.setDimension( b.width() >> 1, b.height() >> 1 );

    for( int j = 0; j < b.height(); ++j ){
        for( int i = 0; i < b.width(); ++i ){
            //
            int x = i << 1;
            int y = j << 1;

            b.r( i, j ) =  o.r( x, y );
            b.g( i, j ) =  o.g( x, y );
            b.b( i, j ) =  o.b( x, y );
            if( o.hasAlpha() ){
                b.a( i, j ) = o.a( x, y );
            }
        }
    }
    swap( o, move(b) );
}

// Here's what drives our function
void contours(Bitmap&o){
    Bitmap b(o);
    grayscale(b);
    binaryGray(o, ISOVALUE);
    auto cont = findContours(b,5);

//    vector<vector<pt>> jm;
//    for( auto& i: cont){
//        jm.push_back(jarvisMarch(i));
//    }
//    for( auto& i: jm){
//        for(auto& j: i){
//            draw( o, j.x, j.y, 0x00FF00, 8);
//        }
//    }

    vector<vector<pt>> hulls;
    for( auto& i: cont ){
        if(i.size() > 3 )
        hulls.push_back(grahamScan(i));
    }
    for( auto& i: hulls){
        for(auto& j: i){
            draw( o, j.x, j.y, 0xFF00FF, 4);
        }
    }

    int count = 0;
    uint32_t color = 0xFF0000;
    for( auto& i: cont){
        for(auto& j: i){
            ++count;
            draw( o, j.x, j.y, color, 2);
        }
        color += 0x001123;
    }
}

vector<vector<pt > > findContours(const Bitmap& o, uint32_t step)
{
    Bitmap b(o);
    binaryGray(b, ISOVALUE);
    // Make it a binary by using a threshold and transforming the entire thing

    // b = binary(b);
    // Contouring cell is represented by 2x2 pixels
    // Compose the 4 bits

    // The grids are independent of other grids
    //binaryGray( b, ISOVALUE);
    transform( o.getBits().begin(), o.getBits().end(), b.getBits().begin(),
        [](auto value){ return (value == 255) ? 1: 0; } );

    // For each bit we'll want to compose it into a new vector
    int32_t w = b.width();
    // ensure against negative height
    int32_t h = b.height()*(b.height() < 0 ? -1:1);

    // Our vector to put things in
    vector<uint8_t> composed(w*h/step, 0);

    // For now, our map of points
    map<pt,pair<edge,edge>,PointEquality<point_t>> points;

    // Some information for moving iterators
    const int32_t bpp = b.bpp();
    const int32_t steps = bpp*step;
    //const int32_t pad = 2;

    // The four corners march fourth on their horses towards the apocalypse
    auto lb = b.getBits().begin()+b.rmask();
    auto lt = lb+w*steps;
    auto rt = lb+w*steps+steps;
    auto rb = lb+steps;

    // Start out with our padding in place
    // We may need to rethink this to make sure that edge detection
    // works for items that go up to the edge of the image.
    // NOTE: we might not even need composed
    //auto ot = composed.begin()+w;
    uint8_t ot;
    //uint32_t leap = bpp*(w*(step-1)+w%step+step); Works on 180x180, not 249x346
    uint32_t leap = bpp*(w*(step-1)+w%step); // 249x346 we should have an extra 1
    if( !(w%step) ){
        leap+=steps;
    }
    int count = 0;
    for( uint32_t i = 0; i < h-step; i+=step )
    {
        for( uint32_t j = 0; j < w-step; j+=step )
        {
            count+=step;
            ot = composeBits({*lt, *rt, *rb, *lb});
            if(ot != 0 && ot != 15){
                // Use *ot to add to pt
                auto vs = edges(ot); // our v's
                auto v = vs.front();  // our first v, but in case of disambiuation we'll do something to it
                if( vs.size() > 1 ){
                    // Do something with these because of ambiguous case
                    // One method is to check the previous one, then do the opposite
                    // If N ? E : W
                    // If E ? N : S
                }
                // map edges into square space from unit square space
                // v.first == v, v.second == v'
                // We'll want Points to be a map from j,i -> edge pairs

                // make_edge gurantees vertexs are ordered
                points.insert(make_pair(pt(j,i),make_pair(
                                     make_edge<point_t>(
                                           pt(j,i)+(v.first.first)*step,
                                           pt(j,i)+(v.first.second)*step
                                          ),
                                     make_edge<point_t>(
                                            pt(j,i)+(v.second.first)*step,
                                            pt(j,i)+(v.second.second)*step
                                           )
                                     ))
                                 );

                cout << pt(j,i) << ": " << to_string(ot) << endl << "e1"
                     << pt(j,i)+(v.first.first)*step
                     << pt(j,i)+(v.first.second)*step
                     << endl << "e2"
                     << pt(j,i)+(v.second.first)*step
                     << pt(j,i)+(v.second.second)*step
                     << endl << endl;
                // in here we want to add our edges to S (the set of all edges)
                // We'll do (e1+(i,j)),(e2+(i,j)) as our edge pairs
            }
            // Increment our horde of iterators
            ++ot; lt+=steps; rt+=steps; lb+=steps; rb+=steps;
            if( lt > b.getBits().end() ){
                cout << "We should never get here." << endl;
            }
            // As long as there is no padding this should be good
        }
        // each time we reach the end of a line we need to jump up steps*width, but also account
        // for the step we already took
        // We are taking the height in steps
        lt+=leap; rt+=leap; lb+=leap; rb+=leap;
        count +=w*(step-1);
        //ot+=pad;
        // cout << endl;
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
       // auto alpha = b.r(i.second.first.first); // sp

        pt e1 = interpolation(i.second.first.first, i.second.first.second, o.r(i.second.first.first), o.r(i.second.first.second), ISOVALUE );
        pt e2 = interpolation(i.second.second.first, i.second.second.second, o.r(i.second.second.first), o.r(i.second.second.second), ISOVALUE );

        interpolated_points[i.first] = make_pair( e1, e2 );
    }

//    transform( points.begin(), points.end(), interpolated_points.begin(), [&b](auto value){
//        auto alpha = b.r(value.second.first.first); // sp

//        pt e1 = interpolation(value.second.first.first, value.second.first.second, alpha );
//        pt e2 = interpolation(value.second.second.first, value.second.second.second, alpha );

//        return make_pair( pt(value.first), make_pair( move(e1), move(e2) ));
//    } );

    vector<vector<pt>> polygons;
    while(!interpolated_points.empty())
    {
        vector<pt> poly = {};
        // Put the first point onto our vector
        // here we have our edges, now what do we do with them?
        // I'm just taking the second edges second endpoint
        // This step is listed as union
        // gamma U next_point

        // Put the point into our polygon
        // Go to first (ignore second for now)
        // 1. Find a point that shares the first edge
        // 2. Place point in bag, remove shared edge, check for next edge
        // 3. ...
        // 4. Profit!!
        // 5. If next edge == current.second, then stop

        // Get the first element by iterator since this is a map, not really anything to
        // Say this is the first
        // For a map it->first == key, it->second == value
        auto it = interpolated_points.begin();
        pt first_pt = it->first;
        poly.emplace_back(first_pt);
        pt last_pt = it->second.second;
        pt current_pt = it->second.first;
        pt cv = first_pt;

        // Remove point
        interpolated_points.erase(it);

        // Then find the next
        bool connected = false;
        while( !connected ){

            vector<pair<pt,pair<pt,pt>>> found;
            for_each( interpolated_points.begin(), interpolated_points.end(),
                      [&current_pt,&found](auto value){
                // Check both edges
                if (value.second.first == current_pt || value.second.second == current_pt){
                    found.push_back(value);
                }
            } );
            if( found.empty()){
                // No points
                cout << "Broken Segment, Starting new Polygon" << endl;
                break;
            }
            auto best_pt = found.back(); found.pop_back();
            auto distance = [](pt p1, pt p2){
              return sqrt(pow(p1.x-p2.x,2)+pow(p1.y-p2.y,2));
            };

            poly.emplace_back(best_pt.first);
            cv = best_pt.first;
            current_pt = ( current_pt == best_pt.second.first ) ? best_pt.second.second : best_pt.second.first;
            interpolated_points.erase(best_pt.first);
            if( current_pt == last_pt ){
                cout << "Finished Building Polygon" << endl;
                connected = true;
            }
        }
        polygons.push_back(poly);
    }
    // The limiting case should be when our step = 1

    return polygons;
}

pt interpolation( pt p, pt q, point_t sp, point_t sq, point_t sigma){
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

void binaryGray( Bitmap &o, const uint32_t isovalue){
    grayscale(o);
    transform(o.getBits().begin(), o.getBits().end(),o.getBits().begin(),
              [&isovalue](auto value){return value > isovalue ? 255 : 0;});
}
uint8_t composeBits( const vector<uint32_t> cell ){
    // cells are  flattened, going from top left to bottom left clockwise
    // Walk around the cell from top left to bottom left, using bitwise OR and
    // left-shift
    uint8_t value = 0;
    for(auto i: cell){
        value <<= 1;
        value |= i;
    }
    if( value == 16 ){
        cout << value << "Should not be 16";
    }
    return value;
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
    /* ******************
     * Bottom, Left
     * +==+
     * |  |
     * -==+
     *******************/
    case 1:
    case 14:
        sides = { make_pair( edge( pt(0,0),pt(0,1) ),
                             edge( pt(0,0),pt(1,0) )
                           )
                };
        break;

    /* ******************
     *  Bottom, Right
     * +==+
     * |  |
     * +==-
     *******************/
    case 2:
    case 13:
        sides = { make_pair( edge( pt(0,0),pt(1,0) ),
                             edge( pt(1,0),pt(1,1) )
                           )
                };
        break;

    /* ******************
     * Left, Right
     * +==+
     * |  |
     * -==-
     *******************/
    case 3:
    case 12:
        sides = { make_pair( edge( pt(0,0),pt(0,1) ),
                             edge( pt(0,1),pt(1,1) )
                           )
                };
        break;

    /* ******************
     * Top, Right
     * +==-
     * |  |
     * +==+
     *******************/
    case 4:
    case 11:
        sides = { make_pair( edge( pt(0,1),pt(1,1) ),
                             edge( pt(1,0),pt(1,1) )
                           )
                };
        break;

    /* ******************
     * {Top, Right}, {Bottom, Left}
     * -==+
     * |  |
     * +==-
     ********************/
    case 5:
    case 10:
        sides = { make_pair( edge( pt(0,1),pt(1,1) ),
                             edge( pt(1,1),pt(1,0) )
                           ),
                  make_pair( edge( pt(0,0),pt(0,1) ),
                             edge( pt(0,0),pt(1,0) )
                           )
                };
        break;

    /* *******************
     * {Top, Bottom}
     * +==-
     * |  |
     * +==-
     *********************/
    case 6:
    case 9:
        sides = { make_pair( edge( pt(0,1),pt(1,1) ),
                             edge( pt(0,0),pt(1,0) )
                           )
                };
        break;

    /* ********************
     * {Top, Left}
     * +==-
     * |  |
     * -==-
     **********************/
    case 7:
    case 8:
        sides = { make_pair( edge( pt(0,1),pt(1,1) ),
                             edge( pt(0,0),pt(0,1) )
                           )
                };
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
