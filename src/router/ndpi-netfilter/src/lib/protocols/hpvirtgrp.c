/*
 * hpvirtgrp.c
 *
 * Copyright (C) 2012-22 - ntop.org
 *
 * This module is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This module is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ndpi_protocol_ids.h"

#define NDPI_CURRENT_PROTO NDPI_PROTOCOL_HPVIRTGRP

#include "ndpi_api.h"
#include "ndpi_private.h"


static void ndpi_int_hpvirtgrp_add_connection(
                struct ndpi_detection_module_struct *ndpi_struct,
                struct ndpi_flow_struct *flow)
{
  ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_HPVIRTGRP, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);
}

static void ndpi_search_hpvirtgrp(struct ndpi_detection_module_struct *ndpi_struct,
                                  struct ndpi_flow_struct *flow)
{
  struct ndpi_packet_struct * packet = ndpi_get_packet_struct(ndpi_struct);

  NDPI_LOG_DBG(ndpi_struct, "search hpvirtgrp\n");

  if (packet->tcp != NULL)
  {
    if (flow->packet_counter == 1 && packet->payload_packet_len >= 4 &&
        packet->payload_packet_len == ntohs(*(u_int16_t*)&packet->payload[1]) &&
        packet->payload[0] == 0x16 && packet->payload[3] == 0x00)
    {
      ndpi_int_hpvirtgrp_add_connection(ndpi_struct, flow);
      return;
    }
  }

  NDPI_EXCLUDE_DISSECTOR(ndpi_struct, flow);
}


/* ***************************************************************** */

void init_hpvirtgrp_dissector(struct ndpi_detection_module_struct *ndpi_struct)
{
  register_dissector("HP Virtual Machine Group Management", ndpi_struct,
                     ndpi_search_hpvirtgrp,
                     NDPI_SELECTION_BITMASK_PROTOCOL_V4_V6_TCP_WITH_PAYLOAD_WITHOUT_RETRANSMISSION,
                     1, NDPI_PROTOCOL_HPVIRTGRP);
}
