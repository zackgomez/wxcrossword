#pragma once
#include <wx/wx.h>
#include <wx/socket.h>
#include "display_panel.hpp"
#include "crossword_board.hpp"

#define MESSAGE_TYPE_BOARD_REQUEST 1
#define MESSAGE_TYPE_BOARD 2
#define MESSAGE_TYPE_UPDATE 3
#define MESSAGE_TYPE_CURSOR 4
#define MESSAGE_TYPE_WIN 5
#define MESSAGE_TYPE_PAUSE 6
#define MESSAGE_TYPE_SOLVE_WORD 7
#define MESSAGE_TYPE_SOLVE_LETTER 8

class crossword_frame : public wxFrame
{
public:
    crossword_frame();
    ~crossword_frame();

    void change();
    void set_letter(int x, int y, char ch);
    void send_cursor(int x, int y, int d);
    void pause();
    void unpause();

private:
    // -- Data Members --
    crossword_board     board_;
    display_panel*      display_;
    wxSocketClient*     socket_;
    wxStatusBar*        status_bar_;

    // -- Event Handlers --
    void on_quit(wxCommandEvent& event);
    void on_socket_event(wxSocketEvent& event);
    void on_connect_menu(wxCommandEvent& event);
    void on_toggle_easy(wxCommandEvent& event);
    void on_solve_word(wxCommandEvent& event);
    void on_solve_letter(wxCommandEvent& event);

    // -- Message Handlers --
    void on_pause(std::string data);
    void on_update(std::string data);
    void on_cursor(std::string data);
    void on_win(std::string data);
    void on_board_data(std::string data);
    void on_connect();
    void on_recieve_data();

    // -- Error Handlers --
    void on_exception(const char* str);

    // -- Helpers --
    std::string create_packet(const std::string& payload, int type) const;
    void send(const std::string& message);
    void set_board(std::istream& in);
    void connect_to_address(wxIPaddress& addr);
};

