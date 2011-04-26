#include "crossword_board.hpp"
#include <stdexcept>
#include <cassert>
#include <sstream>
#include <iostream>

// ----------------- Crossword Clue --------------------------------

/**
 * Creates an empty clue.
 */
crossword_clue::crossword_clue()
: num_(-1), text_(), x_(-1), y_(-1)
{
}

/**
 * Constructor
 */
crossword_clue::crossword_clue(int num, const std::string& text, int x, int y)
: num_(num), text_(text), x_(x), y_(y)
{
}

/**
 * Accessor for the clue number.
 * @return The number of the clue.
 */
int crossword_clue::number() const
{
    return num_;
}

/**
 * Accessor for the clue text.
 * @return The text of the clue.
 */
const std::string& crossword_clue::text() const
{
    return text_;
}

/**
 * Accessor for the x coordinate of the start of the clue.
 * @return The x coord of the clue.
 */
int crossword_clue::x() const
{
    return x_;
}

/**
 * Accessor for the y coordinate of the start of the clue.
 * @return The y coord of the clue.
 */
int crossword_clue::y() const
{
    return y_;
}

/**
 * Equality operator
 */
bool crossword_clue::operator==(const crossword_clue& rhs) const
{
    return rhs.num_ == num_ && rhs.text_ == text_ &&
        rhs.x_ == x_ && rhs.y_ == y_;
}

/**
 * Inequality operator
 */
bool crossword_clue::operator!=(const crossword_clue& rhs) const
{
    return !( *this == rhs );
}

/**
 * This is a static member of crossword_clue representing an empty clue.
 * This object should be returned when a function returning a crossword_clue
 * has no value it would like to return.
 */
const crossword_clue crossword_clue::empty_clue = crossword_clue();

/**
 * Insertion operator for crossword_clues
 */
std::ostream& operator<<(std::ostream& os, const crossword_clue& clue)
{
    os << clue.number() << ": " << clue.text();
    return os;
}

// ------------------ Crossword Board ------------------------------

/// Constant representing the across direction for clues
const int crossword_board::across_dir = 1;
/// Constant representing the down direction for clues
const int crossword_board::down_dir   = 2;
/// Constant representing a wall in the layout array
const int crossword_board::wall_char = -1;

/**
 * Default constructor.
 * Creates an invalid crossword_board object.  To fill in the data and make it
 * usable you need to call crossword_board::read
 */
crossword_board::crossword_board()
: xdim_(0), ydim_(0),
    across_(), down_(),
    letters_(0), layout_(0), answers_(0),
    initialized_(false)
{
}

/// Destructor
crossword_board::~crossword_board()
{
    clear_data();
}

/**
 * Flag indicating whether or not this crossword_board object is ready to use.
 * @return True if the board is usable.
 */
bool crossword_board::initialized() const
{
    return initialized_;
}

/**
 * Flag indicating if the board is in a winning configuration.  This is defined
 * as the entered letters being the same as the answers.
 * @return True if the board is a winner.
 */
bool crossword_board::won() const
{
    for (int i = 0; i < xdim_ * ydim_; i++)
    {
        if (letters_[i] != answers_[i] && (isalpha(answers_[i]) ||
                answers_[i] == ' '))
            return false;
    }

    return true;
}

/**
 * Accessor for the x size of the board.
 * @return The x dimension of the board.
 */
int crossword_board::xdim() const
{
    return xdim_;
}

/**
 * Accessor for the y size of the board.
 * @return The y dimension of the board.
 */
int crossword_board::ydim() const
{
    return ydim_;
}

/**
 * Accessor for the clues in the board.  Returns the clue with the given num or
 * crossword_clue::empty_clue if there is no clue with that number in the given
 * direction.
 * @param dir The direction to check (should be across_dir or down_dir)
 * @param num The number of the clue to get.
 * @return The desired clue or empty_clue if it doesn't exist.
 */
const crossword_clue& crossword_board::clue(int dir, int num) const
{
    assert(dir == down_dir || dir == across_dir); 
    if (dir == across_dir)
    {
        clue_set::const_iterator it = across_.find(num);
        if (it != across_.end())
            return it->second;
    }
    else if (dir == down_dir)
    {
        clue_set::const_iterator it = down_.find(num);
        if (it != down_.end())
            return it->second;
    }
    else
        throw std::invalid_argument("Invalid direction parameter");

    return crossword_clue::empty_clue;
}

/**
 * This function will return the next clue in a direction given a number.  The
 * clue number doesn't have to be a valid clue and the function will wrap around
 * the clue set.
 * @param dir The direction.
 * @param num The number previous to the clue to be found.
 * @return The next clue.
 */
const crossword_clue& crossword_board::next_clue(int dir, int num) const
{
    assert(dir == down_dir || dir == across_dir);

    const clue_set& set = (dir == down_dir) ? down_ : across_;
    clue_set::const_iterator clue_it = set.upper_bound(num);
    if (clue_it == set.end())
        clue_it = set.lower_bound(0);
    
    if (clue_it != set.end())
        return clue_it->second;

    return crossword_clue::empty_clue;
}

/**
 * Accessor for entire sets of clues.
 * @param dir The direction of the set of clues desired.
 * @return A clue_set containing the clues.
 */
const clue_set& crossword_board::clues(int dir) const
{
    assert(dir == down_dir || dir == across_dir);

    if (dir == across_dir)
        return across_; else if (dir == down_dir)
        return down_;

    throw std::invalid_argument("Invalid direction parameter");
}

/**
 * Accessor for the layout information of the board.  This includes clue numbers
 * and walls.  As such this function returns either a clue number, wall_char or 
 * 0 for an unadorned cell.
 * @param x The x coord [0..xdim-1]
 * @param y The y coord [0..ydim-1]
 * @return Adornmends of the cell
 */
int crossword_board::layout_at(int x, int y) const
{
    assert( x < xdim_ && x >= 0 );
    assert( y < ydim_ && y >= 0 );
    return layout_[y * xdim_ + x];
}

/**
 * Accessor the answers to the board.  It will return the character that should
 * be located at that position or '-' for a wall character.
 * @param x The x coord [0..xdim-1]
 * @param y The y coord [0..ydim-1]
 * @return The correct character or '-'
 */
char crossword_board::answer_at(int x, int y) const
{
    assert( x < xdim_ && x >= 0 );
    assert( y < ydim_ && y >= 0 );
    return answers_[y * xdim_ + x];
}

/**
 * Accessor for the letters data.
 * @param x The x coord [0..xdim-1]
 * @param y The y coord [0..ydim-1]
 * @return The current character at the given position.
 */
char crossword_board::at(int x, int y) const
{
    assert( x < xdim_ && x >= 0 );
    assert( y < ydim_ && y >= 0 );
    return letters_[y * xdim_ + x];
}

/**
 * Mutator for the letters data. Returns a non const reference.
 * @param x The x coord [0..xdim-1]
 * @param y The y coord [0..ydim-1]
 * @return The current character at the given position.
 */
char& crossword_board::at(int x, int y)
{
    assert( x < xdim_ && x >= 0 );
    assert( y < ydim_ && y >= 0 );
    return letters_[y * xdim_ + x];
}

/*!
 * Finds the start of a given clue and fills the out params x and y with the
 * location.  You must pass a valid clue or an assert will trigger.
 * @param clue The clue to find the start of.
 * @param x OUT PARAM filled with the xcoord;
 * @param y OUT PARAM filled with the ycoord
 */
void crossword_board::start_of_clue(int clue, int &x, int &y) const
{
    // Iterate over the layout array until we find the start
    for (x = 0; x < xdim_; x++)
    {
        for (y = 0; y < ydim_; y++)
        {
            if (layout_[y * xdim_ + x] == clue)
                return;
        }
    }

    assert(false && "Unable to find clue");
}

/**
 * Reads in all the board data from the given istream.  The expected format is
 * an XML based format of which an example is given in test.xml.
 * @param in The istream to read from.
 * @return True if success.
 */
void crossword_board::read(std::istream& in)
{
    // Remove any old data
    clear_data();


    TiXmlDocument doc;
    in >> doc;

    TiXmlElement *current = doc.FirstChildElement();
    if (!current)
        throw std::runtime_error("The first node is not an element!");

    if (current->Value() == std::string("crossword"))
        current = current->FirstChildElement();

    do
    {
        std::string tag = current->Value();
        if (tag == "Width")
        {
            current->Attribute("v", &xdim_);
            if (xdim_ == 0)
                throw std::runtime_error("Error filling xdim");
        }
        else if (tag == "Height")
        {
            current->Attribute("v", &ydim_);
            if (ydim_ == 0)
                throw std::runtime_error("Error filling ydim");
        }
        else if (tag == "AllAnswer")
        {
            std::string answer_str = current->Attribute("v");
            if (xdim_ == 0 && ydim_ == 0)
                throw std::runtime_error("xdim and ydim values were not filled before AllAnswer element");
            if (xdim_ * ydim_ != static_cast<int>(answer_str.length()))
                throw std::runtime_error("The answer string is of the wrong length");

            allocate_memory();

            for (int i = 0; i < xdim_ * ydim_; i++)
            {
                char ch = answer_str[i];
                answers_[i] = ch;
                if (ch == '-')
                    layout_[i] = wall_char;
            }
        }
        else if (tag == "across")
        {
            TiXmlElement* clue_elem = current->FirstChildElement();
            read_clues(clue_elem, across_);
        }
        else if (tag == "down")
        {
            TiXmlElement* clue_elem = current->FirstChildElement();
            read_clues(clue_elem, down_);
        }
        else if (tag == "letters")
        {
            std::string letter_str = current->Attribute("v");
            if (xdim_ == 0 && ydim_ == 0)
                throw std::runtime_error("xdim and ydim values were not filled before AllAnswer element");
            if (xdim_ * ydim_ != static_cast<int>(letter_str.length()))
                throw std::runtime_error("The letter string is of the wrong length");

            for (int i = 0; i < xdim_ * ydim_; i++)
            {
                char ch = letter_str[i];
                letters_[i] = ch;
            }
        }
            
    } while ((current = current->NextSiblingElement()));
}

// TODO move to a utilities header or something like that
char char_from_hex(const std::string& hex)
{
    std::istringstream blah(hex);
    int ret;
    blah >> std::hex >> ret;
    return ret;
}

// TODO move to a utilities header or something like that
void unescape(std::string& in)
{
    std::string::size_type pos;
    while ((pos = in.find('%')) != std::string::npos)
    {
        std::string hex = in.substr(pos + 1, 2);
        in.replace(pos, 3, 1, char_from_hex(hex));
    }
}

/**
 * Helper function that fills in clues from an XML element containing them.
 * @param parent Should be the Element corresponding to <across> or <down>
 * @param set The clue_set to fill
 */
void crossword_board::read_clues(TiXmlElement* clue_elem, clue_set& set)
{
    TiXmlElement* cur = clue_elem;
    do
    {
        // Clue parts
        int pos, num;
        std::string text;

        cur->Attribute("n", &pos);
        cur->Attribute("cn", &num);
        text = cur->Attribute("c");

        pos--;
        int x = pos % xdim_;
        int y = pos / xdim_;
        unescape(text);
        set[num] = crossword_clue(num, text, x, y);
        layout_[pos] = num;
    } while ((cur = cur->NextSiblingElement()));
}

/**
 * Serializes this board in the xml format to the given ostream.  If letters is
 * true it will include the letters array.  letters=false is useful for saving
 * just the layout/clue information.
 * @param out The stream to write to
 * @param letters If true will include the current letters information.
 */
void crossword_board::write(std::ostream& out, bool letters) const
{
    // Create the doc and add the crossword element that everything goes under
    TiXmlDocument doc;
    TiXmlElement *crossword = new TiXmlElement("crossword");
    doc.LinkEndChild(crossword);
    
    TiXmlElement *width = new TiXmlElement("Width");
    width->SetAttribute("v", xdim_);
    crossword->LinkEndChild(width);

    TiXmlElement *height = new TiXmlElement("Height");
    height->SetAttribute("v", ydim_);
    crossword->LinkEndChild(height);

    TiXmlElement *answers = new TiXmlElement("AllAnswer");
    std::string answer_str(answers_, xdim_ * ydim_);
    answers->SetAttribute("v", answer_str);
    crossword->LinkEndChild(answers);

    TiXmlElement *across = new TiXmlElement("across");
    write_clues(across, across_);
    crossword->LinkEndChild(across);

    TiXmlElement *down = new TiXmlElement("down");
    write_clues(down, down_);
    crossword->LinkEndChild(down);

    if (letters)
    {
        TiXmlElement *letters = new TiXmlElement("letters");
        std::string letter_str(letters_, xdim_ * ydim_);
        letters->SetAttribute("v", letter_str);
        crossword->LinkEndChild(letters);
    }

    out << doc;
}

/**
 * Helper function that writes adds clues to the given parent element.
 * Very similar to the read_clues functions.
 * @param parent Element to add to, should be <across> or <down>
 * @param set The clue_set to write out
 */
void crossword_board::write_clues(TiXmlElement* parent, const clue_set& clues) const
{
    clue_set::const_iterator it = clues.begin();
    for (; it != clues.end(); it++)
    {
        TiXmlElement *clue_elem = new TiXmlElement("clue");
        crossword_clue clue = it->second;
        clue_elem->SetAttribute("c", clue.text());
        clue_elem->SetAttribute("cn", clue.number());
        clue_elem->SetAttribute("n", clue.x() + clue.y() * xdim_ + 1);
        parent->LinkEndChild(clue_elem);
    }
}

// -------------------------------------------------------------------
// Begin Helper Functions
// -------------------------------------------------------------------

/** 
 * Removes all data, deletes any arrays and invalidates the board.
 */
void crossword_board::clear_data()
{
    if (letters_)
        delete[] letters_;
    if (layout_)
        delete[] layout_;
    if (answers_)
        delete[] answers_;
    across_.clear(); down_.clear();
    xdim_ = ydim_ = 0;
    layout_ = 0;
    letters_ = 0;

    initialized_ = false;
}

/**
 * Allocates memory to internal arrays and prepares the board.
 * SHOULD NOT BE CALLED ON A BOARD ALREADY CONTAINING ARRAYS, as this will 
 * result in memory leaks.
 */
void crossword_board::allocate_memory()
{
    letters_ = new char[xdim_ * ydim_];
    layout_ = new int[xdim_ * ydim_];
    answers_ = new char[xdim_ * ydim_];

    if (!letters_ || !layout_ || !answers_)
        throw std::runtime_error("Unable to allocate memory");

    for (int i = 0; i < xdim_ * ydim_; i++)
        answers_[i] = letters_[i] = ' ';

    for (int i = 0; i < xdim_ * ydim_; i++)
        layout_[i] = 0;

    initialized_ = true;
}
