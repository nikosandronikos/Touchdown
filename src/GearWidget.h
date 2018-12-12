#pragma once

#include <vector>
#include <string>
#include <fstream>

typedef struct {
    float x{0.0}, y{0.0};
} Point2;

typedef struct {
    float r{1.0}, g{1.0}, b{1.0};
} Colour;

class GearWidget {
public:
    GearWidget(int numGear, std::vector<Point2> &gearPos, Point2 &bl, Point2 &ur, float width, float height, int labelNLines, std::ofstream &debugFile);
    void draw(float l, float t, float r, float b);
    void setLabel(int n, int line, std::string const &text);
    void setColour(int wheel, float r, float g, float b);

private:
    int nGear;
    float gearWidth;
    float gearHeight;
    std::vector<Point2> gear;
    std::vector<Colour> colours;
    int labelNLines;
    std::vector<std::vector<std::string>> labels;
    std::ofstream &debugFile;
};
