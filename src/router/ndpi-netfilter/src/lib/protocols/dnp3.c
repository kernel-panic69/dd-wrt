/*
 * dnp3.c
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
 * Created by Cesar HM
 *
 */

#include "ndpi_protocol_ids.h"
#define NDPI_CURRENT_PROTO NDPI_PROTOCOL_DNP3
#include "ndpi_api.h"
#include "ndpi_private.h"

/*
  https://www.ixiacom.com/company/blog/scada-distributed-network-protocol-dnp3
*/

/* ******************************************************** */

static void ndpi_search_dnp3_tcp(struct ndpi_detection_module_struct *ndpi_struct,
			  struct ndpi_flow_struct *flow) {
  struct ndpi_packet_struct *packet = ndpi_get_packet_struct(ndpi_struct);

  NDPI_LOG_DBG(ndpi_struct, "search DNP3\n");
    
  if(packet->tcp) {
    /* The payload of DNP3 is 10 bytes long. 
     * Header bytes: 0x0564
     */
    if ((packet->payload_packet_len >= 10)
	&& (packet->payload[0] == 0x05)
	&& (packet->payload[1] == 0x64)) {
      NDPI_LOG_INFO(ndpi_struct, "found DNP3\n");
      ndpi_set_detected_protocol(ndpi_struct, flow, NDPI_PROTOCOL_DNP3, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);
      return;
    }
  }
  NDPI_EXCLUDE_DISSECTOR(ndpi_struct, flow);
   
}

/* ******************************************************** */

void init_dnp3_dissector(struct ndpi_detection_module_struct *ndpi_struct) {

  register_dissector("DNP3", ndpi_struct,
                     ndpi_search_dnp3_tcp,
                     NDPI_SELECTION_BITMASK_PROTOCOL_V4_V6_TCP_WITH_PAYLOAD_WITHOUT_RETRANSMISSION,
                     1, NDPI_PROTOCOL_DNP3);
}
