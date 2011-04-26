#include "display_panel.hpp"
#include <algorithm>
#include <cctype>
#include "crossword_frame.hpp" 
display_panel::display_panel(crossword_frame* parent, crossword_board& board)
: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size), 0),
    parent_(parent),
    changed_(true),
    board_(board),
    easy_mode_(false),
    xcur_(0), ycur_(0), dir_(crossword_board::across_dir), cluenum_(1)
{
    
    // Connect the events
    Connect(wxEVT_PAINT, wxPaintEventHandler(display_panel::on_paint));
    Connect(wxEVT_CHAR, wxKeyEventHandler(display_panel::on_key));
    Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(display_panel::on_mouse));
    Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(display_panel::on_mouse));

    // Create the bitmap buffer
    display_ = new wxBitmap(size, size);

    // The size of the cells in pixels
    x_cellsize_ = size / board_.xdim();
    y_cellsize_ = size / board_.ydim();
}

display_panel::~display_panel()
{
	delete display_;
}

void display_panel::change(bool propagate)
{
    changed_ = true;
    if (propagate)
        parent_->change();
    Refresh();
}

void display_panel::toggle_easy()
{
    easy_mode_ = !easy_mode_;
    change();
}

int display_panel::xcur() const
{
    return xcur_;
}

int display_panel::ycur() const
{
    return ycur_;
}

int display_panel::dir() const
{
    return dir_;
}

int display_panel::cluenum() const
{
    if (changed_)
        update_cluenum();
    return cluenum_;
}

void display_panel::clear_other_cursors()
{
    other_word_coords_.clear();
    change();
}

void display_panel::set_other_cursors(int x, int y, int dir)
{
    update_word_coords(other_word_coords_, x, y, dir, true);
    change();
}

void display_panel::update_display()
{
    wxMemoryDC bitmap(*display_);
    wxBrush black(wxColour(0,0,0));
    wxBrush white(wxColor(255,255,255));
    // Check to see if the board has info in it
    if (!board_.initialized())
    {
        bitmap.SetBackground(white);
        bitmap.Clear();

        bitmap.DrawText(wxT("No Board Loaded"), 40, 40);

        return;
    }
    // Create the device context related things to create the board
    wxBrush yellow(wxColor(255,255,127));
    wxBrush blue(wxColor(150,150,255));
    wxBrush green(wxColor(150,255,150));
    wxBrush overlap(wxColor(200, 255, 255));
    wxColor black_text_color(0, 0, 0);
    wxColor red_text_color(255, 0, 0);

#if defined(_MSC_VER)
    wxFont smallfont = *wxNORMAL_FONT;
    wxFont largefont = smallfont;

    smallfont.SetPixelSize( wxSize(x_cellsize_ * 1 / 6, y_cellsize_ * 1 / 3) );
    largefont.SetPixelSize( wxSize(x_cellsize_ * 1 / 3, y_cellsize_ * 3 / 4) );
    //int xoffset = smallfont.GetPointSize() * 2 / 3 + 1;
    int xoffset = smallfont.GetPixelSize().GetX() * 2 / 3 + 1;
    //int yoffset = smallfont.GetPointSize() / 2 + 1;
    int yoffset = smallfont.GetPixelSize().GetY() / 2 + 1;
#elif defined(LINUX)
    wxFont smallfont = *wxNORMAL_FONT;
    wxFont largefont = smallfont;
    smallfont.SetPointSize( std::min(x_cellsize_, y_cellsize_)  * 1 / 5 );
    largefont.SetPointSize( std::min(x_cellsize_, y_cellsize_)  * 1 / 2 );
    int xoffset = smallfont.GetPointSize() * 2 / 3 + 1;
    int yoffset = smallfont.GetPointSize() / 2 + 1;
#else
    wxFont smallfont = bitmap.GetFont();
    wxFont largefont = smallfont;
    smallfont.SetPointSize( std::min(x_cellsize_, y_cellsize_)  * 1 / 3 );
    largefont.SetPointSize( std::min(x_cellsize_, y_cellsize_)  * 3 / 4 );
    int xoffset = smallfont.GetPointSize() * 2 / 3 + 1;
    int yoffset = smallfont.GetPointSize() / 2 + 1;
#endif

    // Cache these values
    int xdim = board_.xdim();
    int ydim = board_.ydim();

    // Update the woord coords so they draw correctly
    update_word_coords(word_coords_, xcur_, ycur_, dir_, true);

    // Draw each tile.
    for (int y = 0; y < ydim; y++)
    {
        for (int x = 0; x < xdim; x++)
        {
            int cur = board_.layout_at(x, y);
            // Black tile for a wall
            if (cur == crossword_board::wall_char)
            {
                bitmap.SetBrush(black);
                bitmap.DrawRectangle(x * x_cellsize_, y * y_cellsize_,
                                     x_cellsize_, y_cellsize_);
            }
            // It's a non wall
            else
            {
                // Default to white tile for a non-wall
                bitmap.SetBrush(white);
                // Maybe it's the currently selected tile?
                if (x == xcur_ && y == ycur_)
                    // then the color is yellow
                    bitmap.SetBrush(yellow);
                else
                {
                    if (find(word_coords_.begin(), word_coords_.end(),
                                wxPoint(x,y)) != word_coords_.end())
                        bitmap.SetBrush(blue);
                    if (find(other_word_coords_.begin(), other_word_coords_.end(),
                                wxPoint(x,y)) != other_word_coords_.end())
                    {
                        // Is it overlap?
                        if (bitmap.GetBrush() == blue)
                            bitmap.SetBrush(overlap);
                        else
                            bitmap.SetBrush(green);
                    }
                }
                // Draw the background color
                bitmap.DrawRectangle(x * x_cellsize_, y * y_cellsize_,
                                     x_cellsize_, y_cellsize_);

                // Do we need to draw anything in this square?
                // Does a clue start here?
                // If it does draw the number in the top left
                if (cur != 0)
                {
                    wxString text;
                    text << (int) cur;
                    bitmap.SetFont(smallfont);
                    bitmap.DrawText(text, x * x_cellsize_, y * y_cellsize_);
                }
                // Are there solution letters here?
                char cur = board_.at(x, y);
                if (cur != ' ')
                {
                    wxString text;
                    text += cur;
                    // Is it the wrong letter?
                    if (easy_mode_ && board_.answer_at(x, y) != ' ' &&
                            board_.answer_at(x, y) != cur)
                        bitmap.SetTextForeground(red_text_color);
                    bitmap.SetFont(largefont);
                    bitmap.DrawText(text, x * x_cellsize_ + xoffset,
                                    y * y_cellsize_ + yoffset);
                    bitmap.SetTextForeground(black_text_color);
                }
            }
        }
    }

    // Reset changed to false (no changes)
    changed_ = false;
}

bool display_panel::move_velocity(int xvel, int yvel, bool jump)
{
    int newx = xcur_ + xvel;
    int newy = ycur_ + yvel;
    if (newx >= 0 && newx < board_.xdim() && newy >= 0 && newy < board_.ydim())
    {
        if (board_.layout_at(newx, newy) != crossword_board::wall_char)
        {
            xcur_ = newx;
            ycur_ = newy;
            return true;
        }
        // If it does.. and we have jump...
        if (jump)
        {
            int oldx = xcur_;
            int oldy = ycur_;
            
            xcur_ = newx;
            ycur_ = newy;

            if (!move_velocity(xvel, yvel, true))
            {
                xcur_ = oldx;
                ycur_ = oldy;

                return false;
            }
            return true;
        }
    }
    return false;
}

void display_panel::move_direction(bool backward)
{
    int xvel, yvel;
    xvel = yvel = 0;
    
    if (dir_ == crossword_board::down_dir)
        yvel = 1;
    else
        xvel = 1;

    if (backward)
    {
        yvel = -yvel;
        xvel = -xvel;
    }

    move_velocity(xvel, yvel);
}

void display_panel::update_word_coords(std::list<wxPoint>& coords, int x, int y,
        int dir, bool clear)
{
    // Clear the old set
    if (clear)
        coords.clear();
    // Cache the dimension values
    int xdim = board_.xdim();
    int ydim = board_.ydim();

    // First fill in some velocity things so we can use them
    int xvel, yvel;
    xvel = yvel = 0;
    if (dir == crossword_board::down_dir)
        yvel = 1;
    else
        xvel = 1;

    // Now go backwards until the start of the word
    start_of_word(x, y, dir);
    // Now go fowards until we hit a wall or the edge (the end of the word)
    while (x > -1 && x < xdim && y > -1 && y < ydim)
    {
        // have we hit a wall?
        if (board_.layout_at(x, y) == crossword_board::wall_char)
            break;
        // Add the point
        coords.push_back(wxPoint(x, y));
        x += xvel;
        y += yvel;
    }
}

void display_panel::update_cluenum() const
{
    int x = xcur_;
    int y = ycur_;
    start_of_word(x, y, dir_);

    cluenum_ = board_.layout_at(x, y);
}

// TODO Perhaps this is a function better suited for crossword_board
void display_panel::start_of_word(int& x, int& y, int dir) const
{
    int xdim = board_.xdim();
    int ydim = board_.ydim();

    int xvel, yvel;
    xvel = yvel = 0;
    if (dir == crossword_board::down_dir)
        yvel = -1;
    else
        xvel = -1;

    while (x > -1 && x < xdim && y > -1 && y < ydim)
    {
        if (board_.layout_at(x, y) == crossword_board::wall_char)
        {
            x -= xvel;
            y -= yvel;
            return;
        }
        x += xvel;
        y += yvel;
    }
    x -= xvel;
    y -= yvel;
}


void display_panel::on_paint(wxPaintEvent& event)
{
    // Do we need to remake the display bitmap?
    if (changed_)
        update_display();

    // Blit display_ to the screen
    wxPaintDC screen(this);
    wxMemoryDC bitmap(*display_);
    assert(bitmap.IsOk());
    screen.Blit(0, 0, size, size, &bitmap, 0, 0);
}

void display_panel::toggle_dir()
{
    dir_ = (dir_ == crossword_board::down_dir) ? crossword_board::across_dir :
        crossword_board::down_dir;
}


void display_panel::on_key(wxKeyEvent& event)
{
    int code = event.GetKeyCode();
    // Space toggles the clue direction
    if (code == WXK_SPACE)
    {
        toggle_dir();
    }
    // Delete and backspace delete a character
    else if (code == WXK_DELETE || code == WXK_BACK)
    {
        // If the cursor is on a character, then delete that character
        if (board_.at(xcur_, ycur_) != ' ')
        {
            // Delete the character
            parent_->set_letter(xcur_, ycur_, ' ');
        }
        // Otherwise, go back a space and then remove that character
        else
        {
            // Go backwards one space
            move_direction(true);
            parent_->set_letter(xcur_, ycur_, ' ');
        }
    }
    else if (code == WXK_PAUSE || code == WXK_HOME)
    {
        parent_->pause();
    }
    // Tab and Enter go to the next clue
    else if (code == WXK_TAB || code == WXK_RETURN)
    {
        crossword_clue clue;
        if (event.ShiftDown())
            // TODO change this to going backwards
            clue = board_.next_clue(dir_, cluenum_);
        else
            clue = board_.next_clue(dir_, cluenum_);

        // Set the related stuff to be the next clue
        if (clue != crossword_clue::empty_clue)
        {
            cluenum_ = clue.number();
            xcur_ = clue.x();
            ycur_ = clue.y();
        }
    }
    // The directional keys can "jump" over walls
    else if (code == WXK_LEFT)
        move_velocity(-1, 0, true);
    else if (code == WXK_RIGHT)
        move_velocity(1, 0, true);
    else if (code == WXK_UP)
        move_velocity(0, -1, true);
    else if (code == WXK_DOWN)
        move_velocity(0, 1, true);
    else if (isalpha(code))
    {
        // Insert the _uppercase_ character
        char ch = toupper(static_cast<char>(code));
        // Send the update message
        parent_->set_letter(xcur_, ycur_, ch);
        // Go forwards one space
        move_direction();
    }
    else
    {
        event.Skip();
        return;
    }
    parent_->send_cursor(xcur_, ycur_, dir_);
    change(true);
}

void display_panel::on_mouse(wxMouseEvent& event)
{
    int x = event.GetX() / x_cellsize_;
    int y = event.GetY() / y_cellsize_;

    if (x >= 0 && x < board_.xdim() && y >= 0 && y < board_.ydim())
        if (board_.layout_at(x, y) != crossword_board::wall_char)
        {
            if (event.LeftDClick() || (x == xcur_ && y == ycur_))
                toggle_dir();
            else
            {
                xcur_ = x;
                ycur_ = y;
            }
        }
    event.Skip();

    parent_->send_cursor(xcur_, ycur_, dir_);
    change(true);
}
