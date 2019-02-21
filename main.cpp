#include <iostream>
#include <fstream>
#include <string>
#include "bitmap.h"

int main(int argc, char** argv)
{
    if(argc != 4)
    {
        cout << "usage:\n"
             << "bitmap option inputfile.bmp outputfile.bmp\n"
             << "options:\n"
             << "  -i identity\n"
             << "  -c cell shade\n"
             << "  -g gray scale\n"
             << "  -p pixelate\n"
             << "  -b blur\n"
             << "  -r90 rotate 90\n"
             << "  -r180 rotate 180\n"
             << "  -r270 rotate 270\n"
             << "  -v flip vertically\n"
             << "  -h flip horizontally\n"
             << "  -d1 flip diagonally 1\n"
             << "  -d2 flip diagonally 2\n"
             << "  -grow scale the image by 2\n"
             << "  -shrink scale the image by .5" << endl;

        return 0;
    }

    try
    {
        string flag(argv[1]);
        string infile(argv[2]);
        string outfile(argv[3]);

        ifstream in;
        Bitmap image;
        ofstream out;

        in.open(infile, ios::binary);
        in >> image;
        in.close();

        if(flag == "-c")
        {
            cellShade(image);
        }
        if(flag == "-g")
        {
            grayscale(image);
        }
        if(flag == "-p")
        {
            pixelate(image);
        }
        if(flag == "-b")
        {
            blur(image);
        }
        if(flag == "-r90")
        {
            rot90(image);
        }
        if(flag == "-r180")
        {
            rot180(image);
        }
        if(flag == "-r270")
        {
            rot270(image);
        }
        if(flag == "-v")
        {
            flipv(image);
        }
        if(flag == "-h")
        {
            fliph(image);
        }
        if(flag == "-d1")
        {
            flipd1(image);
        }
        if(flag == "-d2")
        {
            flipd2(image);
        }
        if(flag == "-grow")
        {
            scaleUp(image);
        }
        if(flag == "-shrink")
        {
            scaleDown(image);
        }
        if(flag == "-binary")
        {
            binaryGray(image, ISOVALUE);
        }

        out.open(outfile, ios::binary);
        out << image;
        out.close();
    }
    catch(...)
    {
        cout << "Error: an uncaught exception occured." << endl;
    }

    return 0;
}

