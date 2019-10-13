/*
 * app.h
 *
 *  Created on: 25/09/2017
 *      Author: roliveira
 */

#ifndef APP_H_
#define APP_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

class CamWindow;

class MyApp: public wxApp
{
public:
    virtual bool OnInit();
};

class MyFrame: public wxFrame
{
public:
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
private:
    wxPanel*    m_pMainPanel;
    CamWindow * camWindow;

    void OnHello(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);

    wxDECLARE_EVENT_TABLE();
};

#endif /* APP_H_ */
