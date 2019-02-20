/*
 My super useful pixel class
 Think about an ascii art version
*/
class Pixel{
    public:
    float r{0};
    float g{0};
    float b{0};
    float a{0};
    bool hasAlpha{false};

    Pixel( float r, float g, float b, float a=0, bool hasAlpha=false ){};
private:
    friend istream& operator>>(istream& in, Pixel& b);
    friend ostream& operator<<(ostream& out, const Pixel& b);
};
