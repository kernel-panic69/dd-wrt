/*
 * xbox.c
 *
 * Copyright (C) 2016-22 - ntop.org
 *
 * This file is part of nDPI, an open source deep packet inspection
 * library based on the OpenDPI and PACE technology by ipoque GmbH
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

#define NDPI_CURRENT_PROTO NDPI_PROTOCOL_XBOX
#include "ndpi_api.h"
#include "ndpi_private.h"

static void ndpi_int_xbox_add_connection(struct ndpi_detection_module_struct
					 *ndpi_struct, struct ndpi_flow_struct *flow)
{
  ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_XBOX, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);
}


static void ndpi_search_xbox(struct ndpi_detection_module_struct *ndpi_struct, struct ndpi_flow_struct *flow)
{
  struct ndpi_packet_struct *packet = ndpi_get_packet_struct(ndpi_struct);
	
  /*
   * XBOX UDP DETCTION ONLY
   * this detection also works for asymmetric xbox udp traffic
   */
  if(packet->udp != NULL) {

    u_int16_t dport = ntohs(packet->udp->dest);
    u_int16_t sport = ntohs(packet->udp->source);

    NDPI_LOG_DBG(ndpi_struct, "search xbox\n");

    if (packet->payload_packet_len > 12 &&
	get_u_int32_t(packet->payload, 0) == 0 && packet->payload[5] == 0x58 &&
	memcmp(&packet->payload[7], "\x00\x00\x00", 3) == 0) {

      if ((packet->payload[4] == 0x0c && packet->payload[6] == 0x76) ||
	  (packet->payload[4] == 0x02 && packet->payload[6] == 0x18) ||
	  (packet->payload[4] == 0x0b && packet->payload[6] == 0x80) ||
	  (packet->payload[4] == 0x03 && packet->payload[6] == 0x40) ||
	  (packet->payload[4] == 0x06 && packet->payload[6] == 0x4e)) {

	ndpi_int_xbox_add_connection(ndpi_struct, flow);
	NDPI_LOG_INFO(ndpi_struct, "found xbox udp connection detected\n");
	return;
      }
    }
    if ((dport == 3074 || sport == 3074)
	&& ((packet->payload_packet_len == 24 && packet->payload[0] == 0x00)
	    || (packet->payload_packet_len == 42 && packet->payload[0] == 0x4f && packet->payload[2] == 0x0a)
	    || (packet->payload_packet_len == 80 && ntohs(get_u_int16_t(packet->payload, 0)) == 0x50bc
		&& packet->payload[2] == 0x45)
	    || (packet->payload_packet_len == 40 && ntohl(get_u_int32_t(packet->payload, 0)) == 0xcf5f3202)
	    || (packet->payload_packet_len == 38 && ntohl(get_u_int32_t(packet->payload, 0)) == 0xc1457f03)
	    || (packet->payload_packet_len == 28 && ntohl(get_u_int32_t(packet->payload, 0)) == 0x015f2c00))) {
      if (flow->l4.udp.xbox_stage == 1) {
	ndpi_int_xbox_add_connection(ndpi_struct, flow);
	NDPI_LOG_INFO(ndpi_struct, "found xbox udp connection detected\n");
	return;
      }
      NDPI_LOG_DBG(ndpi_struct, "maybe xbox\n");
      flow->l4.udp.xbox_stage++;
      return;
    }
/* Disable this code. These checks are quite weak and these ports are not mentioned at
   https://support.xbox.com/en-US/help/hardware-network/connect-network/network-ports-used-xbox-live */
#if 0
    else if ((dport == 3075 || dport == 3076 || dport == 3077 || dport == 3078) ||
          (sport == 3075 || sport == 3076 || sport == 3077 || sport == 3078)) {
	ndpi_int_xbox_add_connection(ndpi_struct, flow);
	NDPI_LOG_INFO(ndpi_struct, "found xbox udp port connection detected\n");
	return;
    }
#endif
  }

  if(flow->packet_counter >= 5)
    NDPI_EXCLUDE_DISSECTOR(ndpi_struct, flow);
}


void init_xbox_dissector(struct ndpi_detection_module_struct *ndpi_struct)
{
  register_dissector("Xbox", ndpi_struct,
                     ndpi_search_xbox,
                     NDPI_SELECTION_BITMASK_PROTOCOL_V4_V6_UDP_WITH_PAYLOAD,
                     1, NDPI_PROTOCOL_XBOX);
}

