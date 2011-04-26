#pragma once
#include <wx/wx.h>
#include <wx/socket.h>

class connect_dialog : public wxDialog
{
public:
    connect_dialog(const wxString& title = wxT("Connect to server"));

    wxIPV4address address() const;

private:
    wxTextCtrl* hostname_;
    wxTextCtrl* port_;
};
