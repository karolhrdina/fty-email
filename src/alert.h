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

#ifndef ALERT_H_INCLUDED
#define ALERT_H_INCLUDED


#include <algorithm>
#include <fty_proto.h>

#include <cxxtools/serializationinfo.h>



class Alert {
 public:
    Alert () : time(0), last_email_notification(0), last_update(0), last_sms_notification(0) {};
    Alert (fty_proto_t *message) :
            rule (fty_proto_rule (message)),
            element (fty_proto_name (message)),
            state (fty_proto_state (message)),
            severity (fty_proto_severity (message)),
            description (fty_proto_description (message)),
            action (fty_proto_action (message)),
            time (fty_proto_time (message)),
            last_email_notification (0),
            last_update (fty_proto_time (message)),
            last_sms_notification (0)
    {
        std::transform (rule.begin(), rule.end(), rule.begin(), ::tolower);
    };

    bool action_email () { return strcasestr (action.c_str (), "EMAIL") != NULL; }
    bool action_sms () { return strcasestr (action.c_str (), "SMS") != NULL; }

    std::string rule;
    std::string element;
    std::string state;
    std::string severity;
    std::string description;
    std::string action;
    uint64_t time; // when alert started
    uint64_t last_email_notification; // last email notification was sent
    uint64_t last_update; // last time, when alert was changed (for example serevity/status/description)
    uint64_t last_sms_notification; // when last sms notification was sent
};

// Alerts are compared by pair [rule, element]
struct cmpAlertById {
    bool operator() (const Alert& a, const Alert& b) const {
        return (a.rule < b.rule) || (a.rule == b.rule && a.element < b.element);
    }
};
/*
 * \brief Serialzation of Alert
 */
void operator<<= (cxxtools::SerializationInfo& si, const Alert& alert);

/*
 * \brief Deserialzation of Alert
 */
void operator>>= (const cxxtools::SerializationInfo& si, Alert& alert);

void
alert_test (bool verbose);

#endif
