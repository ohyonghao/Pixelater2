#ifndef POINT_HPP
#define POINT_HPP
#include <iostream>

template<typename T>
class point
{
    public:
        T x, y;
        point():x(0), y(0){}
        point(T a, T b):x(a), y(b){}

        point(const point<T> & a):x(a.x), y(a.y){}
        point<T> & operator = (const point<T> & a);
        template<typename U>
        friend std::ostream & operator<< (std::ostream & out,  const point<U> & p);
        template<typename U>
        friend bool operator == (const point<U> & a, const point<U> & b);
        template<typename U>
        friend bool operator != (const point<U> & a, const point<U> & b);

};


template<typename T>
//assignment operator
point<T> & point<T>::operator = (const point<T> & a)
{
    if(this == &a)
        return *this;
    x = a.x;
    y = a.y;
    return *this;
}

template<typename T>
std::ostream & operator<< (std::ostream & out, const point<T> & p)
{
    out << "(" << p.x << ", " << p.y << ")";
    out.flush();

    return out;
}

template<typename T>
bool operator == (const point<T> & a, const point<T> & b)
{
    if((a.x == b.x)&&(a.y == b.y))
        return true;
    else
        return false;
}

template<typename T>
bool operator != (const point<T> & a, const point<T> & b)
{
    if((a.x == b.x)&&(a.y == b.y))
        return false;
    else
        return true;
}


//a comparator that will be used for sorting the points by polar angle around p0
template<typename T>
class Compare{
public:
    Compare(point<T>& p0):_p0{p0}{}
    bool compare(const point<T> & a, const point<T> & b);
    bool operator()(const point<T> & a, const point<T> & b){return compare(a,b);}
    point<T>& p0(){ return _p0;}
private:
    point<T> &_p0;

};
template<typename T>
bool Compare<T>::compare(const point<T> & a, const point<T> & b)
{
    if(colinear(_p0, a, b))
    {
        return distance(_p0, a) < distance(_p0, b);
    }
    else
    {
        if(counterClockWise(_p0, a, b))
            return true;
        else
            return false;
    }
}

#endif // POINT_HPP
