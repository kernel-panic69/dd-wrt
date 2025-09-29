/*
 * non_tcp_udp.c
 *
 * Copyright (C) 2009-11 - ipoque GmbH
 * Copyright (C) 2011-25 - ntop.org
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

#include "ndpi_api.h"
#include "ndpi_private.h"

#define set_protocol_and_bmask(nprot)					\
  {									\
    if (is_proto_enabled(ndpi_struct, nprot))                           \
      {									\
	ndpi_set_detected_protocol(ndpi_struct, flow,			\
				   nprot, NDPI_PROTOCOL_UNKNOWN, NDPI_CONFIDENCE_DPI);		\
      }									\
  }


static void ndpi_search_in_non_tcp_udp(struct ndpi_detection_module_struct
				       *ndpi_struct, struct ndpi_flow_struct *flow)
{
  struct ndpi_packet_struct *packet = ndpi_get_packet_struct(ndpi_struct);

  switch (flow->l4_proto) {
  case NDPI_IPSEC_PROTOCOL_ESP:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_ESP);
    break;

  case NDPI_IPSEC_PROTOCOL_AH:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_AH);
    break;

  case NDPI_GRE_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_GRE);
    break;

  case NDPI_ICMP_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_ICMP);

    if(packet->payload_packet_len < sizeof(struct ndpi_icmphdr)) {
      char buf[64];

      snprintf(buf, sizeof(buf), "Packet too short (%d vs %u)",
               packet->payload_packet_len, (unsigned int)sizeof(struct ndpi_icmphdr));
      ndpi_set_risk(ndpi_struct, flow, NDPI_MALFORMED_PACKET, buf);
    } else {
      u_int8_t icmp_type = (u_int8_t)packet->payload[0];
      u_int8_t icmp_code = (u_int8_t)packet->payload[1];

      /* https://www.iana.org/assignments/icmp-parameters/icmp-parameters.xhtml */
      if(((icmp_type >= 44) && (icmp_type <= 252))
         || (icmp_code > 15)) {
        char buf[64];

        snprintf(buf, sizeof(buf), "Invalid type (%u)/code(%u)",
                 icmp_type, icmp_code);

        ndpi_set_risk(ndpi_struct, flow, NDPI_MALFORMED_PACKET, buf);
      }

      if(packet->payload_packet_len > sizeof(struct ndpi_icmphdr)) {
#ifndef __KERNEL__
        if(ndpi_struct->cfg.compute_entropy && (flow->skip_entropy_check == 0)) {
          flow->entropy = ndpi_entropy(packet->payload + sizeof(struct ndpi_icmphdr),
                                       packet->payload_packet_len - sizeof(struct ndpi_icmphdr));
          ndpi_entropy2risk(ndpi_struct, flow);
        }
#endif
        u_int16_t chksm = icmp4_checksum(packet->payload, packet->payload_packet_len);

        if(chksm) {
          ndpi_set_risk(ndpi_struct, flow, NDPI_MALFORMED_PACKET, "Invalid ICMP checksum");
        }
      }
    }

    break;

  case NDPI_IGMP_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_IGMP);
    break;

  case NDPI_EGP_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_EGP);
    break;

  case NDPI_SCTP_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_SCTP);
    break;

  case NDPI_PGM_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_PGM);
    break;

  case NDPI_OSPF_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_OSPF);
    break;

  case NDPI_IPIP_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_IP_IN_IP);
    break;

  case NDPI_ICMPV6_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_ICMPV6);

    if(packet->payload_packet_len < sizeof(struct ndpi_icmp6hdr)) {
      char buf[64];

      snprintf(buf, sizeof(buf), "Packet too short (%d vs %u)",
               packet->payload_packet_len, (unsigned int)sizeof(struct ndpi_icmp6hdr));

      ndpi_set_risk(ndpi_struct, flow, NDPI_MALFORMED_PACKET, buf);
    } else {
      u_int8_t icmp6_type = (u_int8_t)packet->payload[0];
      u_int8_t icmp6_code = (u_int8_t)packet->payload[1];

      /* https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol_for_IPv6 */
      if(((icmp6_type >= 5) && (icmp6_type <= 127))
         || ((icmp6_code >= 156) && (icmp6_type != 255))) {
        char buf[64];

        snprintf(buf, sizeof(buf), "Invalid type (%u)/code(%u)",
                 icmp6_type, icmp6_code);

        ndpi_set_risk(ndpi_struct, flow, NDPI_MALFORMED_PACKET, buf);
      }
    }

    break;

  case NDPI_PIM_PROTOCOL_TYPE:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_PIM);
    break;

  case 112:
    set_protocol_and_bmask(NDPI_PROTOCOL_IP_VRRP);
    break;
  }

  NDPI_EXCLUDE_DISSECTOR(ndpi_struct, flow);
}


void init_non_tcp_udp_dissector(struct ndpi_detection_module_struct *ndpi_struct)
{
  register_dissector("Non_TCP_UDP", ndpi_struct,
                     ndpi_search_in_non_tcp_udp,
                     NDPI_SELECTION_BITMASK_PROTOCOL_IPV4_OR_IPV6,
                     13,
                     NDPI_PROTOCOL_IP_ESP,
                     NDPI_PROTOCOL_IP_AH,
                     NDPI_PROTOCOL_IP_GRE,
                     NDPI_PROTOCOL_IP_ICMP,
                     NDPI_PROTOCOL_IP_IGMP,
                     NDPI_PROTOCOL_IP_EGP,
                     NDPI_PROTOCOL_IP_SCTP,
                     NDPI_PROTOCOL_IP_PGM,
                     NDPI_PROTOCOL_IP_OSPF,
                     NDPI_PROTOCOL_IP_IP_IN_IP,
                     NDPI_PROTOCOL_IP_ICMPV6,
                     NDPI_PROTOCOL_IP_PIM,
                     NDPI_PROTOCOL_IP_VRRP);
}
