/***
  This file is part of systemd.

  Copyright 2017 Zbigniew Jędrzejewski-Szmek

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <stdbool.h>

#include "time-util.h"

void execute_directories_async(const char* const* directories, usec_t timeout, bool async, char *argv[]);

static inline void execute_directories(const char* const* directories, usec_t timeout, char *argv[]) {
        execute_directories_async(directories, timeout, true, argv);
}
