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
    camera.setMousePosition({pos.x, pos.y});

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

void CamWindow::OnLeftDown(wxMouseEvent& event)
{
    std::cout << "Left click: " << event.GetX() << "," << event.GetY() << std::endl;
    if (camera.hasPerspective()) {
        // send control to robot
        float xPercent = (float)event.GetPosition().x / GetClientSize().x;
        float yPercent = (float)event.GetPosition().y / GetClientSize().y;
        int robotX = screenOffset.first + (xPercent * screenSize.first);
        int robotY = screenOffset.second + (yPercent * screenSize.second);
        std::cout << "mouse x,y percent: "<< xPercent << ", "<< yPercent << std::endl;

        robot.clickXY(robotX, robotY);
    } else {
        camera.pushPerspectiveInputPts({event.GetPosition().x, event.GetPosition().y});
    }
    event.Skip();
}

void CamWindow::OnRightDown(wxMouseEvent& event)
{
    std::cout << "Right click: " << std::endl;
    camera.resetPerspective();
    event.Skip();
}

bool CamWindow::parseConfigFile() {
    std::ifstream is_file(configFile, std::ios::in);
    if (!is_file.is_open()) {
        std::cout << "Failed to open config file: " << configFile << std::endl;
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
    std::cout << "Configuration:" << std::endl;
    std::cout << " > Robot arm length = " << robot.getArmLengthMm() << std::endl;
    std::cout << " > Screen offset = (" << screenOffset.first << ", " << screenOffset.second << ")" << std::endl;
    std::cout << " > Screen size = (" << screenSize.first << ", " << screenSize.second << ")" << std::endl;
    std::cout << std::endl;
    return true;
}
