/*
 * mudfish.c
 *
 * Copyright (C) 2011-25 - ntop.org
 *
 * nDPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * nDPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with nDPI.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ndpi_protocol_ids.h"

#define NDPI_CURRENT_PROTO NDPI_PROTOCOL_MUDFISH

#include "ndpi_api.h"
#include "ndpi_private.h"

static void ndpi_int_mudfish_add_connection(struct ndpi_detection_module_struct * const ndpi_struct,
                                            struct ndpi_flow_struct * const flow)
{
  NDPI_LOG_INFO(ndpi_struct, "found Mudfish\n");
  ndpi_set_detected_protocol(ndpi_struct, flow,
                             NDPI_PROTOCOL_MUDFISH,
                             NDPI_PROTOCOL_UNKNOWN,
                             NDPI_CONFIDENCE_DPI);
}

static void ndpi_search_mudfish(struct ndpi_detection_module_struct *ndpi_struct,
                                struct ndpi_flow_struct *flow)
{
  struct ndpi_packet_struct const * const packet = ndpi_get_packet_struct(ndpi_struct);

  NDPI_LOG_DBG(ndpi_struct, "search Mudfish\n");

  if (packet->udp != NULL)
  {
    if ((packet->udp->source < htons(10000) ||
         packet->udp->source > htons(10010)) &&
        (packet->udp->dest < htons(10000) ||
         packet->udp->dest > htons(10010)))
    {
      NDPI_EXCLUDE_DISSECTOR(ndpi_struct, flow);
      return;
    }

    // Mudfish ping
    if (packet->payload_packet_len == 1 &&
        packet->payload[0] == 0x50)
    {
      if (flow->packet_counter >= 2)
        ndpi_int_mudfish_add_connection(ndpi_struct, flow);
      return;
    }

    NDPI_EXCLUDE_DISSECTOR(ndpi_struct, flow);
    return;
  }

  // Mudfish discovery request
  if (packet->payload_packet_len == 7 &&
      get_u_int32_t(packet->payload, 0) == htonl(0x554e2076) &&
      get_u_int16_t(packet->payload, 4) == htons(0x320d) &&
      packet->payload[6] == 0x0a)
  {
    ndpi_int_mudfish_add_connection(ndpi_struct, flow);
    return;
  }

  // Check discovery response
  if (packet->payload_packet_len > 8 &&
      get_u_int32_t(packet->payload, 0) == htonl(0x554e2041) &&
      get_u_int32_t(packet->payload, 0) == htonl(0x465f494e))
  {
    ndpi_int_mudfish_add_connection(ndpi_struct, flow);
    return;
  }

  NDPI_EXCLUDE_DISSECTOR(ndpi_struct, flow);
}

void init_mudfish_dissector(struct ndpi_detection_module_struct *ndpi_struct)
{
  register_dissector("Mudfish", ndpi_struct,
                     ndpi_search_mudfish,
                     NDPI_SELECTION_BITMASK_PROTOCOL_V4_V6_TCP_OR_UDP_WITH_PAYLOAD_WITHOUT_RETRANSMISSION,
                     1, NDPI_PROTOCOL_MUDFISH);
}
