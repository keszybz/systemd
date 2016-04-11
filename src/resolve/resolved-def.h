#pragma once

/***
  This file is part of systemd.

  Copyright 2014 Lennart Poettering

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

#include <inttypes.h>

#define SD_RESOLVED_DNS                    (UINT64_C(1) << 0)
#define SD_RESOLVED_LLMNR_IPV4             (UINT64_C(1) << 1)
#define SD_RESOLVED_LLMNR_IPV6             (UINT64_C(1) << 2)
#define SD_RESOLVED_MDNS_IPV4              (UINT64_C(1) << 3)
#define SD_RESOLVED_MDNS_IPV6              (UINT64_C(1) << 4)
#define SD_RESOLVED_NO_CNAME               (UINT64_C(1) << 5)
#define SD_RESOLVED_NO_TXT                 (UINT64_C(1) << 6)
#define SD_RESOLVED_NO_ADDRESS             (UINT64_C(1) << 7)
#define SD_RESOLVED_NO_SEARCH              (UINT64_C(1) << 8)
#define SD_RESOLVED_AUTHENTICATED          (UINT64_C(1) << 9)

#define SD_RESOLVED_DNSSEC_DEFAULT         (UINT64_C(0))
#define SD_RESOLVED_DNSSEC_NO              (UINT64_C(1) << 10)
#define SD_RESOLVED_DNSSEC_YES             (UINT64_C(1) << 11)
#define SD_RESOLVED_DNSSEC_ALLOW_DOWNGRADE (SD_RESOLVED_DNSSEC_NO|SD_RESOLVED_DNSSEC_YES)
#define SD_RESOLVED_DNSSEC_OPTIONS         SD_RESOLVED_DNSSEC_ALLOW_DOWNGRADE

#define SD_RESOLVED_LLMNR                  (SD_RESOLVED_LLMNR_IPV4|SD_RESOLVED_LLMNR_IPV6)
#define SD_RESOLVED_MDNS                   (SD_RESOLVED_MDNS_IPV4|SD_RESOLVED_MDNS_IPV6)

#define SD_RESOLVED_PROTOCOLS_ALL          (SD_RESOLVED_MDNS|SD_RESOLVED_LLMNR|SD_RESOLVED_DNS)
#define _SD_RESOLVED_DNSSEC_ALL            (SD_RESOLVED_AUTHENTICATED|_SD_RESOLVED_DNSSEC_MASK)

typedef enum {
        /* These five are returned by dnssec_verify_rrset() */
        DNSSEC_VALIDATED,
        DNSSEC_VALIDATED_WILDCARD, /* Validated via a wildcard RRSIG, further NSEC/NSEC3 checks necessary */
        DNSSEC_INVALID,
        DNSSEC_SIGNATURE_EXPIRED,
        DNSSEC_UNSUPPORTED_ALGORITHM,

        /* These two are added by dnssec_verify_rrset_search() */
        DNSSEC_NO_SIGNATURE,
        DNSSEC_MISSING_KEY,

        /* These two are added by the DnsTransaction logic */
        DNSSEC_UNSIGNED,
        DNSSEC_FAILED_AUXILIARY,
        DNSSEC_NSEC_MISMATCH,
        DNSSEC_INCOMPATIBLE_SERVER,

        _DNSSEC_RESULT_MAX,
        _DNSSEC_RESULT_INVALID = -1
} DnssecResult;

typedef enum {
        DNSSEC_SECURE,
        DNSSEC_INSECURE,
        DNSSEC_BOGUS,
        DNSSEC_INDETERMINATE,

        _DNSSEC_VERDICT_MAX,
        _DNSSEC_VERDICT_INVALID = -1
} DnssecVerdict;
