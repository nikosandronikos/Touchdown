#include "GearWidget.h"

#include "XPLMGraphics.h"

#include <string>
#include <limits>
#if IBM
    #include <windows.h>
#endif

#if LIN
    #include <GL/gl.h>
#elif __GNUC__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


GearWidget::GearWidget(int numGear, std::vector<Point2> &gearPos, Point2 &bl, Point2 &ur, float width, float height, int labelNLines, std::ofstream &debugFile)
:   nGear{numGear} ,
    gearWidth{width/2},
    gearHeight{height/2},
    gear{std::vector<Point2>(nGear)},
    colours{std::vector<Colour>(nGear)},
    labelNLines{labelNLines},
    labels(nGear, std::vector<std::string>(labelNLines)),
    debugFile{debugFile}
{
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::min();
    float minY = minX;
    float maxY = maxX;
    for (int i = 0; i < nGear; i++) {
        if (gearPos[i].x < minX) minX = gearPos[i].x;
        else if (gearPos[i].x > maxX) maxX = gearPos[i].x;
        if (gearPos[i].y < minY) minY = gearPos[i].y;
        else if (gearPos[i].y > maxY) maxY = gearPos[i].y;
        debugFile << "  " << i << " gear: " << gearPos[i].x << ", " << gearPos[i].y << std::endl;
    }
    // Now work out scaling factor and store updated wwheel positions.
    float widgetWidth  = ur.x - bl.x;
    float widgetHeight = ur.y - bl.y;
    float gearWidth    = maxX - minX;
    float gearHeight   = maxY - minY;
    float wRatio       = widgetWidth / gearWidth;
    float hRatio       = widgetHeight / gearHeight;

    Point2 widgetCenter = {bl.x + widgetWidth / 2, bl.y + widgetHeight / 2};
    Point2 gearCenter = {minX + gearWidth / 2, minY + gearHeight / 2};

    debugFile   << "min: " << minX << ", " << minY << std::endl
                << "max: " << maxX << ", " << maxY << std::endl
                << "widgetWidth:  " << widgetWidth << std::endl
                << "widgetHeight: " << widgetHeight << std::endl
                << "gearWidth:  " << gearWidth << std::endl
                << "gearHeight: " << gearHeight << std::endl
                << "wRatio: " << wRatio << std::endl
                << "hRatio: " << hRatio << std::endl
                << "widgetCenter: " << widgetCenter.x << ", " << widgetCenter.y << std::endl
                << "gearCenter: " << gearCenter.x << ", " << gearCenter.y << std::endl;

    for (int i = 0; i < nGear; i++) {
        gear[i].x = (gearPos[i].x - gearCenter.x) * wRatio + widgetCenter.x;
        // Invert y axis to get into UI co-ordinate space.
        gear[i].y = (gearPos[i].y - gearCenter.y) * -hRatio + widgetCenter.y;
        debugFile << "  " << i << " tx: " << gear[i].x << ", " << gear[i].y << std::endl;
    }
}

void GearWidget::draw(float l, float t, float r, float b) {
    float col_white[] = {1.0, 1.0, 1.0}; // red, green, blue
    //debugFile << "draw. l: " << l << "  t:" << t << std::endl;
    for (int i = 0; i < nGear; i++) {
        glColor4f(colours[i].r, colours[i].g, colours[i].b, 1.0f);
        glBegin(GL_QUADS);
        // UI co-ordinate system reverses y axis compared to gear position
        // dataref co-ordinate system. So draw bottom up.
        glVertex2f(gear[i].x - gearWidth + l, b + gear[i].y + gearHeight);
        glVertex2f(gear[i].x + gearWidth + l, b + gear[i].y + gearHeight);
        glVertex2f(gear[i].x + gearWidth + l, b + gear[i].y - gearHeight);
        glVertex2f(gear[i].x - gearWidth + l, b + gear[i].y - gearHeight);
        glEnd();
        for (int j = 0; j < labels[i].size(); j++) {
            XPLMDrawString(
                col_white,
                (int)(l + gear[i].x + gearWidth) + 20,
                (int)(b + gear[i].y - gearHeight - (20 * (j + 1))),
                const_cast<char*>(labels[i][j].c_str()),
                NULL,
                xplmFont_Proportional
            );
        }
    }
}

void GearWidget::setLabel(int n, int line, std::string const &text) {
    labels[n][line] = text;
}

void GearWidget::setColour(int wheel, float r, float g, float b) {
    colours[wheel].r = r;
    colours[wheel].g = g;
    colours[wheel].b = b;
}
