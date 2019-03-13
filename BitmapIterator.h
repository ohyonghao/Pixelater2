#ifndef BITMAPITERATOR_H
#define BITMAPITERATOR_H
#include <iostream>
#include <vector>
class Bitmap;
class BitmapIterator{
public:
    // Our Constructors
    BitmapIterator();
    BitmapIterator(Bitmap*, bool end=false);
    BitmapIterator(const BitmapIterator&);
    BitmapIterator(const BitmapIterator&&);

    // Our Operators
    BitmapIterator& operator=(const BitmapIterator&);
    uint8_t& operator*(){return *_iterator;}
    uint8_t* operator->(){return &*_iterator;}
    BitmapIterator& operator++(){ incWidth(1); _iterator+=_step; return *this;}
    //BitmapIterator& operator--(){ incWidth(-1);_iterator-=_step; return *this;}
    BitmapIterator  operator++(int){auto temp = *this; ++*this; return temp;}
    //BitmapIterator  operator--(int){auto temp = *this; --*this; return temp;}
    BitmapIterator& operator+=(unsigned int i){ incWidth(i); _iterator+=_step*i; return *this;}
    //BitmapIterator& operator-=(unsigned int i){ _iterator-=_step*i; return *this;}
    bool operator!=(const BitmapIterator& rhs)const;
    bool operator==(const BitmapIterator& rhs)const{return _data == rhs._data;}

private:
    Bitmap *_data;

    decltype(std::vector<uint8_t>().begin()) _iterator;
    uint32_t _step;
    int32_t _cwidth{0};
    void incWidth(uint32_t i);
};

template<>
struct std::iterator_traits<BitmapIterator>{
    typedef uint8_t*                          type;
    typedef uint8_t*                          value_type;
    typedef uint8_t**                         pointer;
    typedef uint8_t*&                         reference;
    typedef size_t                            size_type;
    typedef ptrdiff_t                         difference_type;
    typedef std::forward_iterator_tag         iterator_category;
    typedef BitmapIterator                    iterator;
    typedef BitmapIterator                    const_iterator;
};

#endif // BITMAPITERATOR_H
