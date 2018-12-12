#include "GearTracker.h"
#include "GearWidget.h"

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
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

#ifndef XPLM300
#error This is made to be compiled against the XPLM300 SDK
#endif

// An opaque handle to the window we will create
static XPLMWindowID g_window;

// Callbacks we will register when we create our window
void    draw_landing_info(XPLMWindowID in_window_id, void * in_refcon);
int     dummy_mouse_handler(XPLMWindowID in_window_id, int x, int y, int is_down, void * in_refcon) { return 0; }
XPLMCursorStatus    dummy_cursor_status_handler(XPLMWindowID in_window_id, int x, int y, void * in_refcon) { return xplm_CursorDefault; }
int     dummy_wheel_handler(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void * in_refcon) { return 0; }
void    dummy_key_handler(XPLMWindowID in_window_id, char key, XPLMKeyFlags flags, char virtual_key, void * in_refcon, int losing_focus) { }
float   calc_landing_info(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon);

typedef struct {
    char        testMessage[1024];
    std::ofstream    debugFile;
} LandingInfoContext;

GearWidget *view = nullptr;
GearTracker  *model = nullptr;

LandingInfoContext landingInfoCtxt;

PLUGIN_API int XPluginStart( char *outName, char *outSig, char *outDesc) {
    strcpy(outName, "RateMyLanding");
    strcpy(outSig, "xpsdk.examples.rateMyLandingPlugin");
    strcpy(outDesc, "Provides information that allows determination of the quality of the landing.");

    LandingInfoContext *ctxt = &landingInfoCtxt;

    ctxt->debugFile.open("landingInfo.txt");

    std::strcpy(ctxt->testMessage, "LandingInfo started.");
    XPLMRegisterFlightLoopCallback(calc_landing_info, -1, ctxt);

    XPLMCreateWindow_t params;
    params.structSize = sizeof(params);
    params.visible = 1;
    params.drawWindowFunc = draw_landing_info;
    // Note on "dummy" handlers:
    // Even if we don't want to handle these events, we have to register a "do-nothing" callback for them
    params.handleMouseClickFunc = dummy_mouse_handler;
    params.handleRightClickFunc = dummy_mouse_handler;
    params.handleMouseWheelFunc = dummy_wheel_handler;
    params.handleKeyFunc = dummy_key_handler;
    params.handleCursorFunc = dummy_cursor_status_handler;
    params.refcon = ctxt;
    params.layer = xplm_WindowLayerFloatingWindows;
    // Opt-in to styling our window like an X-Plane 11 native window
    // If you're on XPLM300, not XPLM301, swap this enum for the literal value 1.
    params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;

    // Set the window's initial bounds
    // Note that we're not guaranteed that the main monitor's lower left is at (0, 0)...
    // We'll need to query for the global desktop bounds!
    int left, bottom, right, top;
    XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
    params.left = left + 50;
    params.bottom = bottom + 150;
    params.right = params.left + 500;
    params.top = params.bottom + 500;

    g_window = XPLMCreateWindowEx(&params);
    if (g_window == NULL) goto errWindow;

    // Position the window as a "free" floating window, which the user can drag around
    XPLMSetWindowPositioningMode(g_window, xplm_WindowPositionFree, -1);
    // Limit resizing our window: maintain a minimum width/height of 100 boxels and a max width/height of 300 boxels
    XPLMSetWindowResizingLimits(g_window, 200, 200, 500, 500);
    XPLMSetWindowTitle(g_window, "Landing Info");

    return 1;

errWindow:
    return 0;
}

PLUGIN_API void XPluginStop(void)
{
    // Since we created the window, we'll be good citizens and clean it up
    XPLMDestroyWindow(g_window);
    g_window = NULL;
    XPLMUnregisterFlightLoopCallback(calc_landing_info, NULL);

    LandingInfoContext *ctxt = &landingInfoCtxt;

    ctxt->debugFile.close();

    if (view) delete view;
    if (model) delete model;
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int  XPluginEnable(void)  { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam) { }

void draw_landing_info(XPLMWindowID in_window_id, void * in_refcon)
{
    // Mandatory: We *must* set the OpenGL state before drawing
    // (we can't make any assumptions about it)
    XPLMSetGraphicsState(
        0 /* no fog */,
        0 /* 0 texture units */,
        0 /* no lighting */,
        0 /* no alpha testing */,
        1 /* do alpha blend */,
        1 /* do depth testing */,
        0 /* no depth writing */
    );

    int l, t, r, b;
    XPLMGetWindowGeometry(in_window_id, &l, &t, &r, &b);

    float col_white[] = {1.0, 1.0, 1.0}; // red, green, blue

    LandingInfoContext *ctxt = (LandingInfoContext*)in_refcon;

    if (model) {
        model->update();

        if (view) {
            for (int i = 0; i < model->getNGear(); i++) {
                if (model->getGearOnGround(i)) {
                    std::ostringstream fpm, time, force, bounces;
                    fpm << "fpm: " << model->getMaxFpm(i);
                    time << "time: " << model->relativeTime(i);
                    force << "force: " << model->getMaxForce(i);
                    bounces << "bounces: " << model->getBounces(i);
                    view->setColour(i, 0.0, 1.0, 0.0);
                    view->setLabel(i, 0, fpm.str());
                    view->setLabel(i, 1, time.str());
                    view->setLabel(i, 2, force.str());
                    view->setLabel(i, 3, bounces.str());
                } else {
                    view->setColour(i, 0.0, 0.0, 1.0);
                    view->setLabel(i, 0, "");
                    view->setLabel(i, 1, "");
                    view->setLabel(i, 2, "");
                    view->setLabel(i, 3, "");
                }
            }
            view->draw((float)l, (float)t, (float)r, (float)b);
        }

        sprintf(ctxt->testMessage, model->getOnGround() ? "On Ground" : "Airborne");
        XPLMDrawString(col_white, l + 10, t - 20, ctxt->testMessage, NULL, xplmFont_Proportional);
    }


    //ctxt->debugFile << l << ", " << t << " - " << r << ", " << b << std::endl;

}

float calc_landing_info(
    float   inElapsedSinceLastCall,
    float   inElapsedTimeSinceLastFlightLoop,
    int     inCounter,
    void    *inRefcon
) {
    LandingInfoContext *ctxt = (LandingInfoContext*)inRefcon;

    if (model == nullptr) {
        int nGear = currentAircraftNumGear();
        if (nGear > 0) {
            model = new GearTracker(nGear, ctxt->debugFile);
            ctxt->debugFile << std::endl << nGear << " gear identified." << std::endl;
            Point2 bl = {0,0};
            Point2 ur = {200, 200};
            view = new GearWidget(nGear, model->getGearPos(), bl, ur, 10.0, 20.0, 4, ctxt->debugFile);
            ctxt->debugFile << "Initialised. Disabling flight loop callback." << std::endl;
            return 0; // Disable this callback.
        }
    }

    return -1.0;
}
