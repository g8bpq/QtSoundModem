// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

// Syntax: ELEM_(rsid_code, rsid_tag, fldigi_mode)
// fldigi_mode is NUM_MODES if mode is not available in fldigi,
// otherwise one of the tags defined in globals.h.
// rsid_tag is stringified and may be shown to the user.


/*
        ELEM_(263, ESCAPE, NUM_MODES)                   \
*/
#undef ELEM_
#define RSID_LIST                                       \
                                                        \
/* ESCAPE used to transition to 2nd RSID set */         \
                                                        \
        ELEM_(263, EOT, MODE_EOT)                       \
        ELEM_(35, PACKET_300, NUM_MODES)                \
        ELEM_(36, PACKET_1200, NUM_MODES)               \
        ELEM_(155, PACKET_PSK1200, NUM_MODES)           \
                                                        \
        /* NONE must be the last element */             \
        ELEM_(0, NONE, NUM_MODES)

#define ELEM_(code_, tag_, mode_) RSID_ ## tag_ = code_,
enum { RSID_LIST };
#undef ELEM_

#define ELEM_(code_, tag_, mode_) { RSID_ ## tag_, mode_, #tag_ },

const struct RSIDs rsid_ids_1[] = { RSID_LIST };

#undef ELEM_

const int rsid_ids_size1 = sizeof(rsid_ids_1)/sizeof(*rsid_ids_1) - 1;
