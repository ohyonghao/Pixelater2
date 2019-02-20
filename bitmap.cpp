#include <iostream>
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
uint8_t& Bitmap::getPixel( int x, int y, uint32_t mask ){
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
 * Performs a hadamard matrix multiplication (element by element) over two flattened matrixes
 */
void hadamard( const vector<uint32_t>& mult, vector<uint32_t>& result ){
    if( mult.size() != result.size() )
        throw IncompatibleSizeException();

    for( auto i = 0; i < mult.size(); ++i ){
        result[i] *= mult[i];
    }
}

/*
 * Perfom a sum of each element in a flattened matrix
 */
uint32_t matrixSum( const vector<uint32_t>& matrix )noexcept{
    uint32_t sum = 0;
    for( auto i: matrix ){
        sum += i;
    }
    return sum;
}

/*
 * Takes a lower, upper limit and the value to clip
 * @param lower
 * @param upper
 * @param index
 * @return the index value which is clipped to the range [lower, upper)
 */
int clipIndex( int lower, int upper, int index )noexcept{
    int result = (index < lower ? lower : index);
    return ( result >= upper ? upper-1 : result );
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

    const vector<uint32_t> matrix= { 
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
    vector<uint32_t> result[3];
    for( auto& v: result ){
        v.resize(matrix.size());
    }
    for( int j = 0; j < b.height(); ++j ){
        for( int i = 0; i < b.width(); ++i ){
            int xindex = 2;
            int yindex = -2;
            // Load the matrix
            for( int k = 0; k < matrix.size(); ++k ){
                // We'll do it a slow way at first, then think about optimization
                // The biggest roadblock to a good algorithm is optimizing too early
                result[0][k] = b.r( clipIndex(0, b.width(),i+xindex), clipIndex(0, b.height(),j+yindex) );
                result[1][k] = b.g( clipIndex(0, b.width(),i+xindex), clipIndex(0, b.height(),j+yindex) );
                result[2][k] = b.b( clipIndex(0, b.width(),i+xindex), clipIndex(0, b.height(),j+yindex) );
                // Update indexes
                if( yindex == 2 ){
                    --xindex;
                    yindex = -2;
                }else{
                    ++yindex;
                }
            } // k
            for( auto &v: result ){
                hadamard( matrix, v );
            }
            // Now stuff it back in
            gauss.r( i,j ) = clipIndex( 0,256, (matrixSum( result[0] ) >> 8 )) ;
            gauss.g( i,j ) = clipIndex( 0,256, (matrixSum( result[1] ) >> 8 )) ;
            gauss.b( i,j ) = clipIndex( 0,256, (matrixSum( result[2] ) >> 8 )) ;
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
    vector<uint32_t> matrix[3];
    uint32_t result[3] = {0,0,0};
    for( auto& v: matrix ){
        v.resize(16*16);
    }

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
            result[0] = matrixAverage( matrix[0] );
            result[1] = matrixAverage( matrix[1] );
            result[2] = matrixAverage( matrix[2] );
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