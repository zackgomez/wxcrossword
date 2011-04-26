#pragma once
#include <map>
#include <string>
#include "tinyxml.h"

class crossword_clue
{
public:
    // Default constructor creates a "non clue"
    crossword_clue();

    crossword_clue(int num, const std::string& text, int x, int y);

    int number() const;
    const std::string& text() const;
    int x() const;
    int y() const;

    bool operator==(const crossword_clue& rhs) const;
    bool operator!=(const crossword_clue& rhs) const;

    static const crossword_clue empty_clue;

private:
    // Data members
    int num_;
    std::string text_;
    std::string answer_;
    int x_, y_;
};

std::ostream& operator<< (std::ostream& out, const crossword_clue& clue);

// ----------------------------------------------------------
typedef std::map<int, crossword_clue> clue_set;

class crossword_board
{
public:
    // -- Constants --
    static const int across_dir;
    static const int down_dir;
    static const int wall_char;

    // -- Interface Functions --
    // Ctor / Dtor
    crossword_board();
    ~crossword_board();

    // True if the board contains useful information
    bool initialized() const;
    // Returns true if the letters array matches the answers array
    bool won() const;

    // Size accessors
    int xdim() const;
    int ydim() const;

    // Layout accessors
    // This function returns wall_char for a wall
    // 0 for nothing
    // or a number 1 - number of clues for a start of a clue
    int layout_at(int x, int y) const;
    char answer_at(int x, int y) const;

    // User solution accessor/mutator
    char at(int x, int y) const;
    char& at(int x, int y);

    // Clue accessors (invidual clues, entire directions, and size of directions)
    const crossword_clue& clue(int dir, int num) const;
    const crossword_clue& next_clue(int dir, int num) const;
    const clue_set& clues(int dir) const;

    // Gets the x,y coordinate of the start of a clue
    // x, y are out coordinates
    void start_of_clue(int clue, int &x, int &y) const;

    // Board serialization routines
    void read(std::istream& in);
    void write(std::ostream& out, bool letters = true) const;

private:
    // -- Helper functions --
    void read_clues(TiXmlElement* clue_elem, clue_set& set);
    void write_clues(TiXmlElement* parent, const clue_set& set) const;

    void clear_data();
    void allocate_memory();

    // -- Data Members --
    int xdim_, ydim_;
    clue_set across_, down_;
    char *letters_;
    int *layout_;
    char *answers_;
    bool initialized_;
};

