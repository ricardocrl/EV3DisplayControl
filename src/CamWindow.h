/*
 * CamWindow.h
 *
 *  Created on: 02/10/2017
 *      Author: roliveira
 */

#ifndef CAMWINDOW_H_
#define CAMWINDOW_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "Logging.h"
#include "Camera.h"
#include "Robot.h"

wxDECLARE_EVENT(EVT_TRIGGER_FRAME, wxTimerEvent);

class CamWindow : public wxWindow
{
public:
    CamWindow(wxWindow *frame, const wxPoint& pos, const wxSize& size);
    virtual ~CamWindow( );

    void updateSize(const wxSize & newSize);
// private methods
private:
    void OnTriggerFrame(wxTimerEvent& event);

    void    OnPaint(wxPaintEvent& event);
    void    OnLeftDown(wxMouseEvent& event);
    void    OnRightDown(wxMouseEvent& event);

    bool parseConfigFile();
    wxPoint windowToScreenPosition(const wxPoint& windowPos);

    Camera camera;
    Robot robot;
    std::string robotPath;

    std::pair<int, int> screenOffset;
    std::pair<int, int> screenSize;
    const std::string configFile = "cfg/config.ini";

// protected data
protected:
    DECLARE_EVENT_TABLE()
};



#endif /* CAMWINDOW_H_ */
