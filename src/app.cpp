// wxWidgets "Hello world" Program
// For compilers that support precompilation, includes "wx/wx.h".

#include "app.h"

#include "CamWindow.h"

wxDEFINE_EVENT(EVT_REFRESH, wxThreadEvent);

wxIMPLEMENT_APP(MyApp);
bool MyApp::OnInit()
{
    MyFrame *frame = new MyFrame( "EV3 Robot - Remote Touch Control", wxPoint(50, 50), wxSize(450, 340) );
    frame->Show( true );
    return true;
}

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT,  MyFrame::OnExit)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
    EVT_SIZE(MyFrame::OnSize)
wxEND_EVENT_TABLE()

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size), camWindow(NULL)
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(ID_Hello, "&Hello...\tCtrl-H",
                     "Help string shown in status bar for this menu item");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, "&File" );
    menuBar->Append( menuHelp, "&Help" );
    SetMenuBar( menuBar );
    CreateStatusBar();
    SetStatusText( "Welcome to wxWidgets!" );

    m_pMainPanel = new wxPanel(this, -1, wxPoint(0,0), size, 0 );

    // build static/logical boxes
    wxStaticBox *pCameraBox = new wxStaticBox(m_pMainPanel, -1, "camera", wxPoint(2, 0), size);

    // get my main static sizer by the box
    wxStaticBoxSizer *pMainSizer = new wxStaticBoxSizer( pCameraBox, wxVERTICAL );

    camWindow = new CamWindow(this, pCameraBox->GetClientRect().GetTopLeft(), pCameraBox->GetClientSize());
    pMainSizer->Add(camWindow, 1, wxEXPAND);

    m_pMainPanel->SetSizer(pMainSizer);
    pMainSizer->SetSizeHints(m_pMainPanel);
}

void MyFrame::OnExit(wxCommandEvent& event)
{
    Close( true );
}
void MyFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox( "This is a wxWidgets' Hello world sample",
                  "About Hello World", wxOK | wxICON_INFORMATION );
}

void MyFrame::OnSize(wxSizeEvent& event)
{
    std::cout << "CCamView::OnSize" << std::endl;

    if (camWindow) {
        camWindow->updateSize(GetClientSize());
    }

    event.Skip();
    std::cout << "CCamView::OnSize end" << std::endl;
}

