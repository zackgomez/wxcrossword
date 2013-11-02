#include "crossword_frame.hpp"
#include <fstream>
#include <sstream>
#include "connect_dialog.hpp"
#include <stdexcept>

#define ID_CONNECT 101
#define ID_TOGGLE_EASY 102
#define ID_SOLVE_WORD 103
#define ID_SOLVE_LETTER 104

crossword_frame::crossword_frame()
    : wxFrame(NULL, wxID_ANY, wxT("Crossword App"), wxDefaultPosition, wxSize(600, 622),
            wxDEFAULT_FRAME_STYLE & ~ (wxRESIZE_BORDER | wxRESIZE_BORDER | wxMAXIMIZE_BOX)),
    board_(),
    display_(0)
{
    // Setup the socket for later use
    socket_ = new wxSocketClient();
    socket_->SetEventHandler(*this, wxID_ANY);
    socket_->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_CONNECTION_FLAG |
            wxSOCKET_LOST_FLAG);
    socket_->Notify(true);
    socket_->SetFlags(wxSOCKET_WAITALL);

    // Set up the menus
    wxMenuBar* menubar = new wxMenuBar();
    wxMenu* file_menu = new wxMenu();
    file_menu->Append(ID_CONNECT, wxT("Connect to server"));
    file_menu->Append(ID_TOGGLE_EASY, wxT("Toggle easy mode"));
    file_menu->Append(ID_SOLVE_WORD, wxT("Solve clue"));
    file_menu->Append(ID_SOLVE_LETTER, wxT("Solve letter"));
    menubar->Append(file_menu, wxT("&File"));
    SetMenuBar(menubar);

    // Set up the status bar
    status_bar_ = CreateStatusBar();
    status_bar_->SetStatusText(wxT("Not Connected"));

    // Connect the events to the correct handlers
    Connect(ID_CONNECT, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(crossword_frame::on_connect_menu));
    Connect(ID_TOGGLE_EASY, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(crossword_frame::on_toggle_easy));
    Connect(ID_SOLVE_WORD, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(crossword_frame::on_solve_word));
    Connect(ID_SOLVE_LETTER, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(crossword_frame::on_solve_letter));
    Connect(wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(crossword_frame::on_quit));
    Connect(wxEVT_SOCKET, wxSocketEventHandler(crossword_frame::on_socket_event));
}

crossword_frame::~crossword_frame()
{
}

void crossword_frame::change()
{
    if (display_)
    {
        display_->change();

        int cluenum = display_->cluenum();
        int dir = display_->dir();
        int oppdir = dir == crossword_board::across_dir ? crossword_board::down_dir : crossword_board::across_dir;
        // Make sure that we have a valid clue number
        if (cluenum == 0)
            return;

        // Set the current direction's clue
        wxString clue_text(board_.clue(dir, cluenum).text().c_str(), wxConvUTF8);
        wxString status_text;
        status_text << cluenum << wxT(") ") << clue_text;
        this->SetTitle(status_text);

        // Set the opposite direction's clue
        wxString oppclue_text(board_.clue(oppdir, cluenum).text().c_str(), wxConvUTF8);
        status_bar_->SetStatusText(oppclue_text);
    }
}

void crossword_frame::set_letter(int x, int y, char ch)
{
    board_.at(x, y) = ch;
    std::string data;
    data.push_back( static_cast<char>(x) );
    data.push_back( static_cast<char>(y) );
    data.push_back( ch );

    std::string message = create_packet(data, MESSAGE_TYPE_UPDATE);
    send(message);
}

void crossword_frame::send_cursor(int x, int y, int dir)
{
    std::string cursor_data;
    cursor_data.push_back(static_cast<char>(x));
    cursor_data.push_back(static_cast<char>(y));
    cursor_data.push_back(static_cast<char>(dir));

    std::string packet = create_packet(cursor_data, MESSAGE_TYPE_CURSOR);
    send(packet);
}

void crossword_frame::pause()
{
    std::string pause_data;
    pause_data.push_back(static_cast<char>(1));

    std::string packet = create_packet(pause_data, MESSAGE_TYPE_PAUSE);
    send(packet);
}

void crossword_frame::unpause()
{
    std::string pause_data;
    pause_data.push_back(static_cast<char>(0));

    std::string packet = create_packet(pause_data, MESSAGE_TYPE_PAUSE);
    send(packet);
}
void crossword_frame::on_quit(wxCommandEvent& event)
{
    socket_->Notify(false);
    socket_->Destroy();

    Close(true);
}

void crossword_frame::on_pause(std::string data)
{
    if (data.length() != 1)
        return;

    char val = data[0];
    // TODO do this
    if (val)
    {
    }
    else
    {
    }
}

void crossword_frame::on_update(std::string data)
{
    // If the message is the wrong size, simply ignore it
    if (data.size() != 3)
        return;
    int x = static_cast<unsigned char>(data[0]);
    int y = static_cast<unsigned char>(data[1]);
    char ch = data[2];

    // If the message checks out update the board
    if (x >= 0 && x < board_.xdim() && y >= 0 && y < board_.ydim() &&
            (isalpha(ch) || ch == ' '))
        board_.at(x, y) = ch;
}

void crossword_frame::on_cursor(std::string data)
{
    if (data.size() != 3)
        return;
    int x = static_cast<unsigned char>(data[0]);
    int y = static_cast<unsigned char>(data[1]);
    int d = static_cast<unsigned char>(data[2]);

    if (x >= 0 && x < board_.xdim() && y >= 0 && y < board_.ydim() &&
            (d == crossword_board::across_dir || d == crossword_board::down_dir))
        display_->set_other_cursors(x, y, d);
    else if (x == -1 && y == -1 && d == -1)
        display_->clear_other_cursors();
    else
        // Error
        return;
}

void crossword_frame::on_win(std::string data)
{
    int seconds = atoi(data.c_str());
    int minutes = seconds / 60;
    seconds %= 60;
    wxString message = wxString::Format(wxT("Congratulations!\nYou've won in %d:%2d"), minutes, seconds);
    // Tell them they've won
    wxMessageDialog *dialog = new wxMessageDialog(NULL, message,
            wxT("Congratulations"), wxOK);
    dialog->ShowModal();
}



void crossword_frame::on_board_data(std::string data)
{
    std::stringstream ss;
    ss.write(data.c_str(), data.length());

    set_board(ss);
}

void crossword_frame::on_socket_event(wxSocketEvent& event)
{
    if (event.GetSocketEvent() == wxSOCKET_CONNECTION)
    {
        /*
        wxMessageDialog *dialog = new wxMessageDialog(NULL,
                wxT("Connected to server"), wxT("Info"), wxOK);
        dialog->ShowModal();
        */
        on_connect();
    }
    else if (event.GetSocketEvent() == wxSOCKET_LOST)
    {
        wxMessageDialog *dialog = new wxMessageDialog(NULL,
                wxT("Lost connection to server"), wxT("Error"), wxOK);
        dialog->ShowModal();
        // TODO something here so that it doesn't crash
        if (display_)
        {
            delete display_;
            display_ = NULL;
            status_bar_->SetStatusText(wxT("Not Connected"));
        }
    }
    else if (event.GetSocketEvent() == wxSOCKET_INPUT)
    {
        on_recieve_data();
    }
    change();
}

void crossword_frame::on_exception(const char* str)
{
    wxString message(str, wxConvUTF8);

    wxMessageDialog *dialog = new wxMessageDialog(NULL,
            message, wxT("Error"), wxOK);
    dialog->ShowModal();
}


void crossword_frame::on_connect()
{
    std::string message;
    message = create_packet(message, MESSAGE_TYPE_BOARD_REQUEST);

    send(message);
}

void crossword_frame::on_recieve_data()
{
    char header[3];
    socket_->Read(header, 3);
    /*
     * Byte 1 = type
     * Byte 2 & 3 = size
     */
    int type = header[0];
	size_t sizeH = static_cast<size_t>(header[1]) * 256;
	size_t sizeL = static_cast<unsigned char >(header[2]);
    size_t size = sizeH + sizeL;
    std::string payload;
    if (size > 0)
    {
        payload.resize(size);
        socket_->Read((void*) payload.data(), size);
    }

    if (type == MESSAGE_TYPE_BOARD_REQUEST)
    {
        // That's strange, we shouldn't get this... ignore it.
    }
    else if (type == MESSAGE_TYPE_BOARD)
        on_board_data(payload);
    else if (type == MESSAGE_TYPE_UPDATE)
        on_update(payload);
    else if (type == MESSAGE_TYPE_CURSOR)
        on_cursor(payload);
    else if (type == MESSAGE_TYPE_WIN)
        on_win(payload);
    else if (type == MESSAGE_TYPE_PAUSE)
        on_pause(payload);
    else
    {
        // Unknown type
    }
}

void crossword_frame::on_connect_menu(wxCommandEvent& WXUNUSED(event))
{
    // TODO Disconnect or something to deal with being connected
    // Connect the localhost on 3333
    connect_dialog dialog;
	int result = dialog.ShowModal();
    if (result == wxOK || result == wxID_OK)
    {
        wxIPV4address addr = dialog.address();
        connect_to_address(addr);
    }
}

void crossword_frame::on_toggle_easy(wxCommandEvent& WXUNUSED(event))
{
    // Tell the display to toggle easy mode, if it exists
    if (display_)
    {
        display_->toggle_easy();
    }
}

void crossword_frame::on_solve_letter(wxCommandEvent& WXUNUSED(event))
{
    if (!display_)
        return;

    int x = display_->xcur();
    int y = display_->ycur();

    std::string letter_data;
    letter_data.push_back(static_cast<char>(x));
    letter_data.push_back(static_cast<char>(y));

    std::string packet = create_packet(letter_data, MESSAGE_TYPE_SOLVE_LETTER);
    send(packet);
}

void crossword_frame::on_solve_word(wxCommandEvent& WXUNUSED(event))
{
    if (!display_)
        return;

    int clue = display_->cluenum();
    int dir = display_->dir();

    std::string word_data;
    word_data.push_back(static_cast<char>(clue));
    word_data.push_back(static_cast<char>(dir));

    std::string packet = create_packet(word_data, MESSAGE_TYPE_SOLVE_WORD);
    send(packet);
}


std::string crossword_frame::create_packet(const std::string& payload, int type) const
{
    std::string message;
    message.push_back( static_cast<char>(type) );
    int size = payload.length();
    message.push_back( static_cast<char>(size / 256) );
    message.push_back( static_cast<char>(size % 256) );
    message.append(payload);

    return message;
}

void crossword_frame::send(const std::string& message)
{
    if (socket_->WaitForWrite(10) && message.length() < 65535)
        socket_->Write(message.c_str(), message.length());
    else
        // TODO FIX THIS TO REAL ERROR CODE
        assert(false);
}

void crossword_frame::set_board(std::istream& in)
{
    try
    {
        board_.read(in);
    }
    catch (std::runtime_error& e)
    {
        on_exception(e.what());
    }
    // If there is no display, create one
    if (!display_)
    {
        display_ = new display_panel(this, board_);
        SetClientSize(display_->GetSize());
    }

    display_->SetFocus();
}

void crossword_frame::connect_to_address(wxIPaddress& addr)
{
    socket_->Connect(addr, false);
}
