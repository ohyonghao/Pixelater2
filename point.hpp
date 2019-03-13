#ifndef POINT_HPP
#define POINT_HPP
#include <iostream>
#include <utility>

template<typename T>
class point
{
    public:
        T x, y;
        point():x(0), y(0){}
        point(T a, T b):x(a), y(b){}

        point(const point<T> & a):x(a.x), y(a.y){}
        point( const point<T> &&a):x{a.x}, y{a.y}{}
        point<T> & operator = (const point<T> & a);
        point<T> & operator = (const point<T> && a){this->x = std::move(a.x);
                                                    this->y = std::move(a.y);
                                                    return *this;
                                                   }
        template<typename U>
        friend std::ostream & operator<< (std::ostream & out,  const point<U> & p);
        template<typename U>
        friend bool operator == (const point<U> & a, const point<U> & b);
        template<typename U>
        friend bool operator != (const point<U> & a, const point<U> & b);

        point<T> operator+ (const point<T> & rhs){ return point<T>(this->x+rhs.x, this->y+rhs.y); }
        point<T>& operator+=(const point<T> & rhs){this->x += rhs.x; this->y += rhs.y; return *this; }
        point<T> operator* (const T &rhs){ return point<T>(this->x*rhs, this->y*rhs); }
        template<typename U>
        friend point<U> operator* (const point<U>& lhs, const U &rhs);

};

template<typename T>
point<T> operator*( const point<T>& lhs, const T &rhs){ return(lhs.x*rhs, lhs.y*rhs); }

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
    Compare(const point<T>& p0):_p0{p0}{}
    bool compare(const point<T> & a, const point<T> & b);
    bool operator()(const point<T> & a, const point<T> & b){return compare(a,b);}
    const point<T>& p0()const{ return _p0;}
    point<T>& p0(){return _p0;}
private:
    point<T> _p0;

};
// Think about a lambda creating function like this
// function<bool(point,point)> makefun(point p0){return [p0](point p1, point p2){...};
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

template<typename T>
class PointEquality{
public:
    size_t operator()(const point<T>& lhs, const point<T>& rhs )const{
        return ( lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y)  );
    }
};


typedef double point_t;
typedef point<point_t> pt;
typedef std::pair<pt,pt> edge;

template<typename T>
edge make_edge( point<T>&& lhs, point<T>&& rhs ){
    if( PointEquality<T>()(lhs, rhs) ){
        return edge(lhs,rhs);
    }
    else{
        return edge(rhs,lhs);
    }
}

#endif // POINT_HPP
