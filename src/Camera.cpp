/*
 * workeropencv.cpp
 *
 *  Created on: 25/09/2017
 *      Author: roliveira
 */

#include "Camera.h"

#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types_c.h>

Camera::Camera(wxEvtHandler *owner, int timerid) :
	wxTimer(owner, timerid), initialized(false) {
     perspectiveInputPts.push_back(cv::Point(0,0));
}

Camera::~Camera() {
    capture.release();
}

Camera::Size Camera::init()
{
    capture.open(0);
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 2000);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 2000);

    // grab first frame
    captureFrame();
    setOutputSize({image.GetSize().x, image.GetSize().y});

    initialized = true;
    return {outputSize.width, outputSize.height};
}

bool Camera::captureFrame() {
    capture.read(frame);

    if (frame.empty()) {
    	return false;
    }

	// Transform or resize
	// After init(), outputSize will be always set
	if (initialized) {
		if (!transformMtx.empty()) {
			cv::warpPerspective(frame, frame, transformMtx, outputSize);
		} else {
			cv::resize(frame, frame, outputSize);
		}
	}
	// if no video image
	cv::cvtColor(frame, frame, CV_BGR2RGB);

	// Draw selected polygon
	if ((perspectiveInputPts.size() > 1) && (perspectiveInputPts.size() <= 4)) {
		int vertexCount = perspectiveInputPts.size();
		const cv::Point * polyPts = perspectiveInputPts.data();
		cv::fillPoly(frame, &polyPts, &vertexCount, 1, cv::Scalar(255, 255, 255));
	}

	// convert data from raw image to wxImg
	image = wxImage(frame.size[1], frame.size[0], frame.data, true);

	return true;
}

void Camera::pushPerspectiveInputPts(const Camera::Point& pt) {
    if (perspectiveInputPts.size() < 5) {
        perspectiveInputPts.push_back(cv::Point2f(static_cast<float>(pt.x), static_cast<float>(pt.y)));
        updateTransformMtx();
    } else {
        LOG("Camera::pushPerspectiveInputPts" << "number of input points exceeded");
    }
}

void Camera::setOutputSize(const Camera::Size& outSize) {
    outputSize = cv::Size(outSize.w, outSize.h);
    perspectiveOutputPts = {
            {0,0},
            {(float)outSize.w, 0},
            {(float)outSize.w, (float)outSize.h},
            {0, (float)outSize.h}};
    updateTransformMtx();
}

void Camera::resetPerspective() {
    transformMtx.release();
    perspectiveInputPts = std::vector<cv::Point>(1, cv::Point(0,0));
}

void Camera::updateMousePosition(const Camera::Point& pos) {
	perspectiveInputPts[0] = cv::Point(pos.x, pos.y);
}

void Camera::updateTransformMtx() {
    // 1 of the elements in perspectiveInputPts is the mouse position on the 0th idx, for drawing purposes.
    if ((perspectiveInputPts.size() == 5) && (perspectiveOutputPts.size() == 4)) {

        std::vector<cv::Point2f> inputToMtx(std::next(perspectiveInputPts.begin()), perspectiveInputPts.end());
        transformMtx = cv::getPerspectiveTransform(inputToMtx.data(), perspectiveOutputPts.data());
    }
}

bool Camera::hasPerspective() {
    return !transformMtx.empty();
}
