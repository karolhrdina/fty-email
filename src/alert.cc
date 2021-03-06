/*  =========================================================================
    alert - Alert representation

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

/*
@header
    alert - Alert representation
@discuss
@end
*/

#include "fty_email_classes.h"

/*
 * \brief Serialzation of Alert
 */
void operator<<= (cxxtools::SerializationInfo& si, const Alert& alert)
{
    si.addMember("rule") <<= alert.rule;
    si.addMember("element") <<= alert.element;
    si.addMember("state") <<= alert.state;
    si.addMember("severity") <<= alert.severity;
    si.addMember("description") <<= alert.description;
    si.addMember("time") <<= alert.time;
    si.addMember("last_update") <<= alert.last_update;
    // TODO consider to rename this in state file
    si.addMember("last_notification") <<= alert.last_email_notification;
    si.addMember("action") <<= alert.action;
    si.addMember("last_sms_notification") <<= alert.last_sms_notification;
}

/*
 * \brief Deserialzation of Alert
 */
void operator>>= (const cxxtools::SerializationInfo& si, Alert& alert)
{
    si.getMember("rule") >>= alert.rule;
    si.getMember("element") >>= alert.element;
    si.getMember("state") >>= alert.state;
    si.getMember("severity") >>= alert.severity;
    si.getMember("description") >>= alert.description;
    si.getMember("time") >>= alert.time;
    si.getMember("last_update") >>= alert.last_update;
    si.getMember("last_notification") >>= alert.last_email_notification;
    try {
        si.getMember ("action") >>= alert.action;
    }
    catch (const cxxtools::SerializationError &e) {
        alert.action = "EMAIL/SMS";
    }
    try {
        si.getMember ("last_sms_notification") >>= alert.last_sms_notification;
    }
    catch (const cxxtools::SerializationError &e) {
        alert.last_sms_notification = 0;
    }
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
alert_test (bool verbose)
{
    printf (" * alert: ");
    
    Alert a;
    assert ( a.action_sms() == false );
    assert ( a.action_email() == false );
    a.action = "EMAIL/SMS";
    assert ( a.action_sms() == true );
    assert ( a.action_email() == true );
    //  @selftest
    //  @end
    printf ("OK\n");
}
