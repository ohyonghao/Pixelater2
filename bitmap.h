#ifndef MY_BITMAP_H_
#define MY_BITMAP_H_
#include <iostream>
#include <vector>
#include <memory>
#include <exception>
#include <cmath>
#include "point.hpp"
#include "BitmapIterator.h"
/*
Tasks to do:
1. Create a Pixel class to hold the argb pixel information, this should be simple
2. Load a bitmap file into Bitmap
3. Start implementing the helper functions, one at a time

I suppose the loading of the bitmap file is done by the iostream operators which have been declared
by our benevolent dictator as friends to our class.

Let's first make our pixel class
*/

using namespace std;

//typedef point<double> fpt;

const int32_t ISOVALUE = 57;
const int32_t STEPSIZE = 5;
class Bitmap
{
private:
    friend istream& operator>>(istream& in, Bitmap& b);
    friend ostream& operator<<(ostream& out, const Bitmap& b);
    // Bitmap file format header:

    // 14 bytes wide
    // Hex Dec Size Purpose
    // 00  0   2B   Id type of bitmap
    // 02  2   4B   Size of BMP
    // 06  5   4B   Garbage
    // 0A  10  4B   Offset to the start of data
    // Warning: Read in little endian if using Vim:%!xxd
#pragma pack(push, 2) // fixes problem with char array becoming 4 bytes for alignment purposes
    struct HEADER{
        // This solution, which is now commented out, does work, and technically worked without needing to 
        // Even have the padding in here as alignment takes care of that for us by aligning to 8 bytes, the
        // compiler inserted already 2 bytes between ftype and size, or before ftype like I did.
        // Upon looking at gcc's documentation on this, the reason for GCC supporting pragma is for
        // compatibility with MS compilers. Dealing with this without pragma's resulted in special knowledge
        // for certain operations which I'll leave commented to show the other solution. One bit of special
        // knowledge would be for when using sizeof() you would be getting the 2byte too large size.
        // Arguably this would be alright, unless I expose the internal header to the outside, though users
        // of this should also have access to this header file and be reading this nicely written explanation,
        // or looking it up on Wikipedia and Googling for reasons why a BMP header is only 14 Bytes, but is
        // showing up 16 Bytes.
//        private:
//        char        PADDING[2];     // Padding added to align DWORD
//        public:
        char        ftype[2];       // file type, should be 0x42 0x4D, or BM in ASCII
        uint32_t    size;           // size of BMP in bytes
        uint16_t    res1;           // Reserved; application dependent
        uint16_t    res2;           // Reserved; application dependent
        uint32_t    offset;         // Offset to the start of data
    }header;
#pragma pack(pop) // not needed for other structures

    // Hex Dec Size Purpose
    // 0E  14  4B   size of the second header
    // 12  18  4B   width in pixels (signed)
    // 16  22  4B   height in pixels (signed)
    // 1A  26  2B   number of color planes
    // 1C  28  2B   the color depth of the image
    // 1E  30  4B   the compression method being used
    // 22  34  4B   the size of the raw bitmap data.
    // 26  38  4B   the horizontal resolution of the image (for printers)
    // 2A  42  4B   the vertical resolution of the image   (for printers)
    // 2E  46  4B   colors in color palate
    // 32  50  4B   the number of important colors used

    // Compression 0 means bitdepth will be 24
    // Compression 3 means bitdepth will be 32

    struct DIBs{
        uint32_t    size;           // Size of this header
        int32_t     width;          // Width of image
        int32_t     height;         // Height of image
        uint16_t    cPlanes;        // Number of color planes
        uint16_t    cDepth;         // Color depth
        uint32_t    cmpsn;          // Compression Method
        uint32_t    rawSize;        // Size of raw bitmap data
        uint32_t    phres;          // Horizontal Resolution (Printers)
        uint32_t    pvres;          // Vertical Resolution (Printers)
        uint32_t    cPalate;        // Colors in palate
        uint32_t    impColor;       // Number of important colors
    }dibs;

    // Color Space information - only used when Compression mode is 3
    // Hex Dec Size Purpose
    // 36  54  4B   mask 1
    // 3A  58  4B   mask 2
    // 3E  62  4B   mask 3
    // 42  66  4B   mask 4
    // 46  70  4B   the order of the masks
    // 4A  74  64B  Color space information (we can ignore this)
    struct COLORSPACE{
        uint32_t        mask[4];    // indexed array
        uint32_t        masko;      // Order of mask
        uint32_t        info[16];   // Can be ignored
    }colorspace;

    uint32_t r_mask = 0;
    uint32_t g_mask = 0;
    uint32_t b_mask = 0;
    uint32_t a_mask = 0;

    
    uint32_t _rowSize  = 0;  // Row Size in Bytes
    uint32_t _rowWidth = 0;  // Row Width in Pixels including padding
    uint32_t _bpp      = 0;  // Bytes per pixel

    // This is where we store everything
    vector<uint8_t>  _bits;

    // Helper private functions
    // Uses info in colorspace to init masks
    void setmask( );

    // Returns the position in the mask where a mask is a disjoint set of bitshifted 0xFF values
    uint32_t maskToInt( uint32_t )noexcept;

    // Returns single pixel/color
    uint8_t& getPixel( int x, int y, uint32_t mask );
    const uint8_t& getPixel( int x, int y, uint32_t mask )const;

public:
    Bitmap() = default;
    Bitmap(const Bitmap&, bool noData = false);
    Bitmap& operator=( const Bitmap& rhs ) = default;
    Bitmap(Bitmap&&) = default;
    //~Bitmap();

    uint8_t& r( int x, int y ){ return getPixel( x, y, r_mask ); }
    uint8_t& g( int x, int y ){ return getPixel( x, y, g_mask ); }
    uint8_t& b( int x, int y ){ return getPixel( x, y, b_mask ); }
    uint8_t& a( int x, int y ){ return getPixel( x, y, a_mask ); }

    uint8_t& r( pt& p ){ return getPixel( p.x, p.y, r_mask ); }
    uint8_t& g( pt& p ){ return getPixel( p.x, p.y, g_mask ); }
    uint8_t& b( pt& p ){ return getPixel( p.x, p.y, b_mask ); }
    uint8_t& a( pt& p ){ return getPixel( p.x, p.y, a_mask ); }

    const uint8_t& r( pt& p )const{ return getPixel( p.x, p.y, r_mask ); }
    const uint8_t& g( pt& p )const{ return getPixel( p.x, p.y, g_mask ); }
    const uint8_t& b( pt& p )const{ return getPixel( p.x, p.y, b_mask ); }
    const uint8_t& a( pt& p )const{ return getPixel( p.x, p.y, a_mask ); }

    int32_t  height() const{ return dibs.height < 0 ? -dibs.height: dibs.height ; }
    int32_t  width() const{ return dibs.width; }
    uint32_t  rowWidth() const{return _rowWidth; }
    void     setHeight( int32_t height ){ setDimension( dibs.width, height); }
    void     setWidth( int32_t width ){ setDimension( width, dibs.height); }
    uint32_t rmask() const{ return r_mask; }
    uint32_t gmask() const{ return g_mask; }
    uint32_t bmask() const{ return b_mask; }
    uint32_t amask() const{ return a_mask; }
    bool     hasAlpha() const{ return dibs.cmpsn; }
    uint32_t padding(){return _rowWidth-_rowSize;}

    auto& getBits(){return _bits;}
    const auto& getBits()const{return _bits;}
    auto bpp(){ return _bpp;}
    // This function sets the internal dimensions of the bitmap, and in doing so
    // it takes no regards for the image that was in it and should be considered
    // corrucpted.
    void setDimension( int32_t width, int32_t height );
    friend void swap( Bitmap&, Bitmap&& );

    // Iterator Traits
    typedef uint8_t                                 type;
    typedef uint8_t                                 value_type;
    typedef uint8_t*                                pointer;
    typedef uint8_t&                                reference;
    typedef size_t                                  size_type;
    typedef ptrdiff_t                               difference_type;
    typedef std::bidirectional_iterator_tag         iterator_category;
    typedef BitmapIterator                          iterator;
    typedef BitmapIterator                          const_iterator;

    // Iterators
    iterator begin(){return iterator(this);}
    iterator end(){return iterator(this,true);}
    iterator cbegin(){return iterator(this);}
    iterator cend(){return iterator(this,true);}
};
class BadFileTypeException: public exception{
    inline const char * what() const noexcept{
        return "Not Bitmap type";
    }
};

class IncompatibleSizeException: public exception{
    inline const char * what() const noexcept{
        return "Hadamard Product Requires Same Size Matrices, You Idiot!";
    }
};

class BadMaskOrderException: public exception{
    inline const char * what() const noexcept{
        return "Is not BGR or s";
    }
};

class OutOfBoundsException: public exception{
    inline const char * what() const noexcept{
        return "Coordinates out of bounds";
    }
};

class InvalidWidthException: public exception{
    inline const char * what() const noexcept{
        return "Width must be greater than 0";
    }
};

// Filter Functions
void cellShade(Bitmap& b)noexcept;
void grayscale(Bitmap& b);
void pixelate(Bitmap& b);
void blur(Bitmap& b);
void rot90(Bitmap& b);
void rot180(Bitmap& b);
void rot270(Bitmap& b);
void flipv(Bitmap& b);
void fliph(Bitmap& b)noexcept;
void flipd1(Bitmap& b);
void flipd2(Bitmap& b);
void scaleUp(Bitmap& b);
void scaleDown(Bitmap& b);

void contours(Bitmap& b, int32_t isovalues=ISOVALUE, int32_t stepsize=STEPSIZE, bool useBinaryBitmap = true);

// Final Functions

/*!
 * \brief binaryGray
 * \param image is the canvas to be changed to binary gray
 * \param isovalue the grayscale value to split between negative and positive (0, 1)
 */
void binaryGray( Bitmap &image, const uint32_t isovalue);
/*!
 * \brief findContours returns a vector of vectors of points, that is to say that
 *        each vector is a set of points that should form a completed contour
 * \param o
 * \return vector of sets of points that create a completed contour shape
 */
vector<vector<pt>> findContours(const Bitmap& o, int32_t isovalue, uint32_t step, bool useBinaryInterp);
/*!
 * \brief edges lookup table for 2^4 edge possibilities
 * \param square a binary value with positions 0,1,2,3 being the corners of a square from
 * the marching squares algorithm. lb, rb, rt, lt with lt being the most significant.
 * \return a vector of edge pairs from the table
 */
vector<pair<edge,edge>> edges( uint8_t square );
/*!
 * \brief composeBits
 * \param b the previous corner
 * \param b2 the new corner 2 after one step
 * \param b3 the new corner 3 after one step
 * \return previous corners 2, 3 moved to 1,4, and new corners 2,3 inserted in.
 */
uint8_t composeBits(uint8_t b, uint8_t b2, uint8_t b3 );
pt interpolation(pt p, pt q, point_t sp , point_t sq, point_t sigma);

// Drawing functions
void draw(Bitmap&o, uint32_t x, uint32_t y , uint32_t color, uint32_t thickness = 10);
void drawLine(Bitmap & o, const pt & p1, const pt & p2, uint32_t color, uint32_t thickness);

#endif // MY_BITMAP_H_
