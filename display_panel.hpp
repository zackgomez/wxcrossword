#pragma once
#include <wx/wx.h>
#include <list>
#include "crossword_board.hpp"

class crossword_frame;

class display_panel : public wxPanel
{
public:
    // -- Constants --
    static const int size = 600;

    display_panel(crossword_frame* parent, crossword_board& board);
	~display_panel();

    void change(bool propagate = false);
    void toggle_easy();
    void clear_other_cursors();
    void set_other_cursors(int x, int y, int dir);

    int xcur() const;
    int ycur() const;
    int dir() const;
    int cluenum() const;

private:
    // Data members
    crossword_frame *parent_;
    wxBitmap *display_;
    // True if the board has changed since the last update of display
    bool changed_;

    // The board!
    const crossword_board& board_;
    // The size in px of the cells
    int x_cellsize_, y_cellsize_;
    // The current x and y coords of the cursor
    int xcur_, ycur_;
    // And the current direction
    int dir_;
    // The current clue number
    mutable int cluenum_;
    // Whether or not to highlight wrong clues
    bool easy_mode_;
    // A set containing the currently selected word's coords
    std::list<wxPoint> word_coords_;
    // A set containing the currently selected words from other people
    std::list<wxPoint> other_word_coords_;

    // Helpers
    void update_display();
    void update_word_coords(std::list<wxPoint>& coords, int x, int y, int dir,
            bool clear);
    void start_of_word(int& x, int& y, int dir) const;
    void move_direction(bool backward = false);
    bool move_velocity(int xvel, int yvel, bool jump = false);
    void update_cluenum() const;
    void toggle_dir();

    // Event handlers
    void on_paint(wxPaintEvent& event);
    void on_key(wxKeyEvent& event);
    void on_mouse(wxMouseEvent& event);
};

