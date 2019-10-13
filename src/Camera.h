/*
 * workeropencv.h
 *
 *  Created on: 25/09/2017
 *      Author: roliveira
 */

#ifndef CAMERA_H_
#define CAMERA_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <opencv2/videoio.hpp>

//////////////////////////////////////////////////
// class:   CX10PingWorker
/////////////////////////////////////////////////
class Camera : public wxTimer
{
// methods
public:
    struct Point {
        int x;
        int y;
    };
    struct Size {
        int w;
        int h;
    };

    Camera(wxEvtHandler *owner, int timerid);
    ~Camera();

    Camera::Size init();
    bool captureFrame();

    const wxImage& getImage() const {
        return image;
    }

    bool hasPerspective();
    void pushPerspectiveInputPts(const Camera::Point& pt);
    void setPerspectiveInputPts(const std::vector<Camera::Point>& pts);
    void resetPerspective();
    void setMousePosition(const Camera::Point& pos);

    void setOutputSize(const Camera::Size& size);

private:
    void updateTransformMtx();

    bool initialized;

    cv::VideoCapture capture;
    wxImage image;


    wxSize size;
    cv::Mat frame;
    cv::Mat transformMtx;
    std::vector<cv::Point> perspectiveInputPts;
    std::vector<cv::Point2f> perspectiveOutputPts;
    cv::Size outputSize;
};



#endif /* CAMERA_H_ */
