/*  =========================================================================
    Copyright (C) 2014 - 2017 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

#ifndef ELEMENTLIST_H_INCLUDED
#define ELEMENTLIST_H_INCLUDED

#include <string>
#include <map>

class Element {
 public:

    std::string name;
    uint8_t priority;
    std::string contactName;
    std::string email;
    std::string sms_email;
    std::string phone;

    void debug_print () const;
};

class ElementList
{
 public:
    ElementList () : _path(), _path_set(false) {};
    ElementList (const std::string& path_to_file) : _path(path_to_file), _path_set(true) {};

    // returns
    //  * true - element with 'asset_name' exists and is assigned to 'element'
    //  * false - element with 'asset_name' does not exist and 'element' is not changed
    bool    get (const std::string& asset_name, Element& element) const;
    void    add (const Element& element);
    void    remove (const char *asset_name);
    bool    exists (const std::string& asset_name) const;
    bool    empty () const;
    void    setFile (const std::string& path_to_file);
    void    setFile ();
    int     save (); // TODO prepsat, tohle je strasny
    int     load (const std::string &sms_gateway); // TODO prepsat, tohle je strasny
    std::string serialize_to_json () const;
    void    updateContactName (const std::string &elementName, const std::string &contactName);
    void    updateEmail (const std::string &elementName, const std::string &email);
    void    updateSMSEmail (const std::string &elementName, const std::string &email);
    void    updatePhone (const std::string &elementName, const std::string &phone);
    unsigned int size(void) const;
 private:
    std::map <std::string, Element> _assets;
    std::string _path;
    bool _path_set;

    static const std::string DEFAULT_PATH_TO_FILE;
};

//  Self test of this class
void
    elementlist_test (bool verbose);
//  @end


#endif // ELEMENTLIST_H_INCLUDED
