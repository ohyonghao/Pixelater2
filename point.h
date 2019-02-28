#ifndef POINT_H
#define POINT_H
#include <iostream>
class point
{
    public:
        int x, y;
        point():x(0), y(0){}
        point(int a, int b):x(a), y(b){}

        point(const point & a):x(a.x), y(a.y){}
        point & operator = (const point & a);
        friend std::ostream & operator<< (std::ostream & out,  const point & p);
        friend bool operator == (const point & a, const point & b);
        friend bool operator != (const point & a, const point & b);

};

#endif // POINT_H
