/*
 * teamspeak.c 
 *
 * Copyright (C) 2013 Remy Mudingay <mudingay@ill.fr>
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
 */

#include "ndpi_protocol_ids.h"

#define NDPI_CURRENT_PROTO NDPI_PROTOCOL_TEAMSPEAK

#include "ndpi_api.h"
#include "ndpi_private.h"

static void ndpi_int_teamspeak_add_connection(struct ndpi_detection_module_struct
                                              *ndpi_struct, struct ndpi_flow_struct *flow)
{
  ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_TEAMSPEAK, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);
}


static void ndpi_search_teamspeak(struct ndpi_detection_module_struct *ndpi_struct, struct ndpi_flow_struct *flow)
{
  struct ndpi_packet_struct *packet = ndpi_get_packet_struct(ndpi_struct);

  NDPI_LOG_DBG(ndpi_struct, "search teamspeak\n");

  if (packet->payload_packet_len >= 20) {
    if (packet->udp != NULL) {
      if (memcmp(packet->payload, "TS3INIT1", strlen("TS3INIT1")) == 0)
      {
        NDPI_LOG_INFO(ndpi_struct, "found TEAMSPEAK udp\n");
        ndpi_int_teamspeak_add_connection(ndpi_struct, flow);
        return;
      }
    } else if(packet->tcp != NULL) {
      /* https://github.com/Youx/soliloque-server/wiki/Connection-packet */
      if(((memcmp(packet->payload, "\xf4\xbe\x03\x00", 4) == 0)) ||
         ((memcmp(packet->payload, "\xf4\xbe\x02\x00", 4) == 0)) ||
         ((memcmp(packet->payload, "\xf4\xbe\x01\x00", 4) == 0)))
      {
        NDPI_LOG_INFO(ndpi_struct, "found TEAMSPEAK tcp\n");
        ndpi_int_teamspeak_add_connection(ndpi_struct, flow);
        return;
      }  /* http://www.imfirewall.com/en/protocols/teamSpeak.htm  */
    }
  }

  if (packet->udp != NULL)
  {
    if (packet->payload_packet_len == 16 &&
        packet->payload[0] == 0x01 && packet->payload[3] == 0x02 &&
        get_u_int32_t(packet->payload, 11) == 0x00000000 && packet->payload[15] == 0x00)
    {
      goto ts3_license_weblist;
    }

    if ((packet->payload_packet_len == 4 || packet->payload_packet_len == 8) &&
        packet->payload[0] == 0x01 && packet->payload[3] == 0x01)
    {
      goto ts3_license_weblist;
    }

    if (packet->payload_packet_len == 5 &&
        packet->payload[0] == 0x01 && packet->payload[3] == 0x02 &&
        packet->payload[4] == 0x00)
    {
      goto ts3_license_weblist;
    }
  }

  NDPI_EXCLUDE_DISSECTOR(ndpi_struct, flow);
  return;

ts3_license_weblist:
  if (flow->packet_counter == 3)
  {
    NDPI_LOG_INFO(ndpi_struct, "found TEAMSPEAK license/weblist\n");
    ndpi_int_teamspeak_add_connection(ndpi_struct, flow);
    return;
  }
}

void init_teamspeak_dissector(struct ndpi_detection_module_struct *ndpi_struct)
{
  register_dissector("TeamSpeak", ndpi_struct,
                     ndpi_search_teamspeak,
                     NDPI_SELECTION_BITMASK_PROTOCOL_V4_V6_TCP_OR_UDP_WITH_PAYLOAD_WITHOUT_RETRANSMISSION,
                     1, NDPI_PROTOCOL_TEAMSPEAK);
}

