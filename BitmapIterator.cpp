#include "BitmapIterator.h"
#include "bitmap.h"

BitmapIterator::BitmapIterator():
    _data{nullptr},
    _iterator{},
    _step{0}
{

}

BitmapIterator::BitmapIterator(Bitmap* data, bool end):
    _data{data},
    _iterator{},
    _step{data->bpp()}
{
    _iterator = end ? data->getBits().end() : data->getBits().begin();
}

bool BitmapIterator::operator!=(const BitmapIterator& rhs)const{
    return !(!(*this == rhs) || (_iterator == _data->getBits().end()));
}

void BitmapIterator::incWidth(uint32_t i){
    _cwidth+=i;
    while(_cwidth >= _data->width()){
        _cwidth   -= _data->width();
        _iterator += _data->padding();
    }
}
