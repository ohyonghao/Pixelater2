#ifndef JARVISMARCH_H
#define JARVISMARCH_H

#include <iostream>
#include <vector>
#include <stack>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include "point.hpp"

template<typename T>
std::vector<point<T>> jarvisMarch(std::vector<point<T>>&);
template<typename T>
std::vector<point<T>> grahamScan(std::vector<point<T>>&);

//square distance between two points
template<typename T>
int distance(const point<T> & p1, const point<T> & p2)
{
	int val = (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
	return val;
}

template<typename T>
//check for CCW - counter clock wise
bool counterClockWise(const point<T> & p1, const point<T> & p2, const point<T> & p3)
{
    int32_t result = (p2.y - p1.y) * (p3.x - p2.x) - (p2.x - p1.x) * (p3.y - p2.y);
	if(result < 0)
		return true;
	else
		return false;
}

//check for colinear
template<typename T>
bool colinear(const point<T> & p1, const point<T> & p2, const point<T> & p3)
{
    int32_t result = (p2.y - p1.y) * (p3.x - p2.x) - (p2.x - p1.x) * (p3.y - p2.y);
	return result == 0;
}

//check for CW - clock wise
template<typename T>
bool ClockWise(const point<T> & p1, const point<T> & p2, const point<T> & p3)
{
	int result = (p2.y - p1.y) * (p3.x - p2.x) - (p2.x - p1.x) * (p3.y - p2.y);
	if(result > 0)
		return true;
	else
		return false;
}

/*
//a comparator that will be used for sorting the points by polar angle around p0
bool compare(const point & a, const point & b)
{
	if(colinear(p0, a, b))
	{
		return distance(p0, a) > distance(p0, b);
	}
	else
	{
		if(counterClockWise(p0, a, b))
			return false;
		else
			return true;
	}
}
*/

//to compare the top 2 element of the stack S with point pi
//we need a function to get the second top element from the stack

template<typename T>
point<T> secondTopE(std::stack<point<T>> & s)
{
	point p = s.top();
	s.pop();

	point secondTop = s.top();
	s.push(p);

	return secondTop;
}

//jarvis march algorithm
template<typename T>
std::vector<point<T>> jarvisMarch(std::vector<point<T>> & points)
{
	int n = points.size();
	//if number of input <= 3 the input is a convex hull
	if( n <= 3)
	{
		return points;
	}
    std::vector<point<T>> convexHull;
	
	//starts with find the left most point
	int leftMost = 0;
	for(int i = 1; i < n; ++i)
	{
		if(points[i].x < points[leftMost].x)
			leftMost = i;
	}

	//travel counterclockwise to find the rightmost point of a current point. 
	int p = leftMost, q;
 	do
	{
		q = (p + 1) % n;
		for(int i = 0; i < n; ++i)
		{
			if(counterClockWise(points[p], points[i], points[q]))
				q = i;
		}
		convexHull.push_back(points[q]);
		p = q;
	}while(p != leftMost);

	return convexHull;
}

template<typename T>
void swap(point<T>& lhs, point<T>& rhs ){
    point<T> temp = rhs;
    rhs = lhs;
    lhs = temp;
}
//graham scan algorithm
template<typename T>
std::vector<point<T>> grahamScan(std::vector<point<T>> & points)
{
	int n = points.size();
    std::vector<point<T>> convexHull;

	if(n <= 3)
		return points;

	//find the point which has the lowest y-coordinate
    T min_y = points[0].y;
    T Min = 0;
	for(int i = 1; i < n; ++i)
	{
        T cur_y = points[i].y;
		if(cur_y < min_y ||((min_y == cur_y) && (points[i].x < points[Min].x)))
		{
			Min = i;
			min_y = points[i].y;
		}
	}

	//switch p0 and pmin so p0 can have the lowest y-coordinate
	swap(points[0], points[Min]);

	//sort the points by polar angle around p0;
    // Make a compare class here and set the point
    Compare compare(points[0]);

    sort(points.begin()+1, points.end(), compare);


	int arrSize = 1;    //used to locate items in modified array
   	for(int i = 1; i<n; i++) 
	{

      //when the angle of ith and (i+1)th elements are same, remove points
            while(i < n-1 && colinear(compare.p0(), points[i], points[i+1]))
         		i++;
      		points[arrSize] = points[i];
      		arrSize++;
   	}

   	if(arrSize < 3)
      		return convexHull;  	


    std::stack<point<T>> S;
	S.push(points[0]);
	S.push(points[1]);
	S.push(points[2]);

	for(int i = 3; i < arrSize; ++i)
//	for(int i = 3; i < n; ++i)

	{
		while(counterClockWise(secondTopE(S), S.top(), points[i]) == false)
		{
			S.pop();
		}
		S.push(points[i]);
	}
	
	//move the points in the stack to convexHull vector
	while(!S.empty())
	{
		convexHull.push_back(S.top());
		S.pop();
	}
		
	return convexHull;
}

//function to generate randam points 
template<typename T>
std::vector<point<T>> randomPoints(int n)
{
    std::vector<point<T>> points(n);
	int x, y;

	//different values each time the program is run	
	srand(time(NULL)); 
	for(int i = 0; i < n; ++i)
	{
		x = rand() % 100;
		y = rand() % 100;	

        point<T> p;
		p.x = x;
		p.y = y;

		points[i] = p;
	}
	return points;
}

// a function to print the points
template<typename T>
void print(std::vector<point<T>> & points)
{
    Compare compare(points[0]);
	sort(points.begin(), points.end(), compare);

	for(auto point:points)
	{
        std::cout << point << std::endl;
	}

    std::cout << "size = " << points.size() << std::endl;
}

template<typename T>
void writexTXT(char file_name[], std::vector<point<T>> & points)
{
    std::ofstream file_out;

	file_out.open(file_name);

	if(!file_out)
		return;
	if(file_out)
	{
		for(auto point:points)
		{
			file_out << point.x << "\n";
		}
	}
	file_out.close();
}
template<typename T>
void writeyTXT(char file_name[], std::vector<point<T>> & points)
{
    std::ofstream file_out;

	file_out.open(file_name);

	if(!file_out)
		return;
	if(file_out)
	{
		for(auto point:points)
		{
			file_out << point.y << "\n";
		}
	}
	file_out.close();
}
//construct input having all the points on the convex hull
template<typename T>
std::vector<point<T>> pointsOnHull(int n)
{
    std::vector<point<T>> result(n);
	for(int i = 0; i < n; ++i)
	{
		result[i].x = i;
		result[i].y = i*i;
	}
	return result;
}
/*
int main()
{
    std::vector<point> points = {{-7,8},{-4,6},{2,6},{6,4},{8,6},{7,-2},{4,-6},{8,-7},{0,0},
                     {3,-2},{6,-10},{0,-6},{-9,-5},{-8,-2},{-8,0},{-10,3},{-2,2},{-10,4}};
    //	std::vector<point> points = { { 0, 3 }, { 1, 1 }, { 2, 2 }, { 4, 4 }, { 0, 0 }, { 1, 2 }, { 3, 1 }, { 3, 3 } };
    std::vector<point> convexHull_j, convexHull_g;

	convexHull_j = jarvisMarch(points);
	convexHull_g = grahamScan(points);


//	print(convexHull_j);
//	cout << endl << endl;
//	print(convexHull_g);

//	return 0;

	
    //std::vector<point> rPoints(48);
	//rPoints = randomPoints(48);
	
//	writexTXT("x.txt", rPoints);
//	writeyTXT("y.txt", rPoints);
//
    std::vector<point> rPoints = pointsOnHull(2000);

	//unique(rPoints.begin(), rPoints.end());

//	cout << "random points " << endl;
//	for(auto p:rPoints)
//	{
//		cout << p << endl;
//	}

	cout << "size = " << rPoints.size() << endl;
	
	
	//track the time for jarvis march
	clock_t t1;
	t1 = clock();

    std::vector<point> convexHull_J= jarvisMarch(rPoints);

	t1 = clock() - t1;

	cout<<"\n\njarvis march took "<<t1<<" clicks "<<((float)t1)/CLOCKS_PER_SEC
	<<" seconds"<<endl << endl;

	//track the time for grahamScan
	clock_t t2;
	t2 = clock();

    std::vector<point> convexHull_G = grahamScan(rPoints);

	t2 = clock() - t2;

	cout<<"graham scan took "<<t2<<" clicks "<<((float)t2)/CLOCKS_PER_SEC
	<<" seconds"<<endl << endl;
	
//	writexTXT("hullx.txt", convexHull_G);
//	writeyTXT("hully.txt", convexHull_G);


//	print(convexHull_J);
//	cout<<endl<<endl<<endl;
//	print(convexHull_G);	

	cout <<"size of jarvis is: " << convexHull_J.size() << " size of graham is: " << convexHull_G.size() << endl;

    //std::vector<point> hull = pointsOnHull(10);
	//print(hull);
	return 0;
}


*/
#endif // JARVISMARCH_HPP
