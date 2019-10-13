/*
 * CamWindow.cpp
 *
 *  Created on: 02/10/2017
 *      Author: roliveira
 */

#include <fstream>

#include "CamWindow.h"

wxDEFINE_EVENT(EVT_TRIGGER_FRAME, wxTimerEvent);

BEGIN_EVENT_TABLE(CamWindow, wxWindow)
    EVT_PAINT(CamWindow::OnPaint)
    EVT_TIMER(EVT_TRIGGER_FRAME, CamWindow::OnTriggerFrame)
    EVT_LEFT_DOWN(CamWindow::OnLeftDown)
    EVT_RIGHT_DOWN(CamWindow::OnRightDown)
END_EVENT_TABLE()

CamWindow::CamWindow(wxWindow *frame, const wxPoint& pos, const wxSize& size ) :
    wxWindow(frame, -1, pos, size, wxSIMPLE_BORDER), camera(this, EVT_TRIGGER_FRAME), robot() {

    if (!parseConfigFile()) {
        exit(1);
    }

    while (!robot.connect(robotPath)) {
        sleep(2);
    }

    if (!robot.goHome()) {
        exit(1);
    }

    Camera::Size camSize = camera.init();
    SetSize(camSize.w, camSize.h);
    frame->SetSize(frame->GetSize() - frame->GetClientSize() + wxSize(camSize.w, camSize.h));
    camera.Start(20, true);
}

CamWindow::~CamWindow() {
}

void CamWindow::updateSize(const wxSize & newSize) {
    camera.setOutputSize({newSize.x, newSize.y});
    SetSize(newSize.x, newSize.y);
}

void CamWindow::OnTriggerFrame(wxTimerEvent& event) {
    auto pos = wxGetMousePosition() - GetScreenPosition();
    camera.updateMousePosition({pos.x, pos.y});

    if (camera.captureFrame()) {
        Refresh();
    }
    camera.Start(20, true);
}

void CamWindow::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);
    wxImage image = camera.getImage();
    if (image.IsOk()) {
        wxBitmap currentBitmap(image);

        int x, y, w, h;
        dc.GetClippingBox(&x, &y, &w, &h);
        dc.DrawBitmap(currentBitmap, x, y);
    }
}

wxPoint CamWindow::windowToScreenPosition(const wxPoint& windowPos) {
    float xPercent = (float)windowPos.x / GetClientSize().x;
    float yPercent = (float)windowPos.y / GetClientSize().y;
    int screenX = screenOffset.first + (xPercent * screenSize.first);
    int screenY = screenOffset.second + (yPercent * screenSize.second);
    return wxPoint(screenX, screenY);
}

void CamWindow::OnLeftDown(wxMouseEvent& event)
{
    LOG("Left click: " << event.GetX() << "," << event.GetY());
    if (camera.hasPerspective()) {

    	wxPoint screenPos = windowToScreenPosition(event.GetPosition());
        robot.clickXY(screenPos.x, screenPos.y);

    } else {
        camera.pushPerspectiveInputPts({event.GetPosition().x, event.GetPosition().y});
    }
    event.Skip();
}

void CamWindow::OnRightDown(wxMouseEvent& event)
{
    LOG("Right click: ");
    camera.resetPerspective();
    event.Skip();
}

bool CamWindow::parseConfigFile() {
    std::ifstream is_file(configFile, std::ios::in);
    if (!is_file.is_open()) {
        LOG("Failed to open config file: " << configFile);
        return false;
    }

    std::string line;
    while (std::getline(is_file, line)) {
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string valueString;
            if (std::getline(is_line, valueString)) {

                if (key == "arm_length") {

                	int valueInt = std::stoi(valueString);
                    if (valueInt > 0) {
                        robot.setArmLengthMm(valueInt);
                    }
                } else if (key == "screen_offset_x") {

                    screenOffset.first = std::stoi(valueString);

                } else if (key == "screen_offset_y") {

                    screenOffset.second = std::stoi(valueString);

                } else if (key == "screen_width") {

                	int valueInt = std::stoi(valueString);
                    if (valueInt > 0) {
                        screenSize.first = valueInt;
                    }
                } else if (key == "screen_height") {

                	int valueInt = std::stoi(valueString);
                    if (valueInt > 0) {
                        screenSize.second = valueInt;
                    }
                } else if (key == "robot_path") {
                	robotPath = valueString;
                }
            }
        }
    }
    LOG("Configuration:");
    LOG(" > Robot arm length = " << robot.getArmLengthMm());
    LOG(" > Screen offset = (" << screenOffset.first << ", " << screenOffset.second << ")");
    LOG(" > Screen size = (" << screenSize.first << ", " << screenSize.second << ")");
    LOG(" > Robot path = (" << robotPath << ")");
    std::cout << std::endl;
    return true;
}
