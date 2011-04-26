#include "connect_dialog.hpp"
#include <sstream>

connect_dialog::connect_dialog(const wxString& title)
    : wxDialog(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize)
{
    hostname_ = new wxTextCtrl(this, wxID_ANY, wxT("127.0.0.1"));
    port_ = new wxTextCtrl(this, wxID_ANY, wxT("3333"));

    wxStaticText *host_text = new wxStaticText(this, wxID_ANY, wxT("Address: "));
    wxStaticText *port_text = new wxStaticText(this, wxID_ANY, wxT("Port: "));

    wxButton *ok_button = new wxButton(this, wxID_OK, wxT("&Ok"));
    wxButton *cancel_button = new wxButton(this, wxID_CANCEL, wxT("&Cancel"));

    wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->Add(cancel_button);
    button_sizer->Add(ok_button, 0, wxLEFT, 15);

    wxBoxSizer *info_sizer = new wxBoxSizer(wxHORIZONTAL);
    info_sizer->Add(host_text);
    info_sizer->Add(hostname_);
    info_sizer->Add(port_text,0, wxLEFT, 20);
    info_sizer->Add(port_);

    wxBoxSizer *everything_sizer = new wxBoxSizer(wxVERTICAL);
    everything_sizer->Add(info_sizer, 1, wxTOP | wxRIGHT | wxLEFT, 20);
    everything_sizer->Add(button_sizer, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM | wxRIGHT | wxLEFT, 20);

    SetSizer(everything_sizer);
}

wxIPV4address connect_dialog::address() const
{
    int port = 0;
    wxString port_string = port_->GetLineText(0);
    port = atoi(port_string.mb_str());

    wxIPV4address addr;
    addr.Hostname(hostname_->GetLineText(0));
    addr.Service(port);

    return addr;
}
