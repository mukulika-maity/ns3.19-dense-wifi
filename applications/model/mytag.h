/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#ifndef MYTAG_H
#define MYTAG_H

#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <iostream>

namespace ns3 {

class MyTag : public Tag
{

private:
  uint8_t m_simpleValue;
public:
  static TypeId GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyTag")
    .SetParent<Tag> ()
    .AddConstructor<MyTag> ()
    .AddAttribute ("SimpleValue",
                   "A simple value",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&MyTag::GetSimpleValue),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}
  virtual TypeId GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
  virtual uint32_t GetSerializedSize (void) const
{
  return 1;
}
  virtual void Serialize (TagBuffer i) const
{
  i.WriteU8 (m_simpleValue);
}
  virtual void Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU8 ();
}
  virtual void Print (std::ostream &os) const
{
  os << "v=" << (uint32_t)m_simpleValue;
}
  void SetSimpleValue (uint8_t value)
{
  m_simpleValue = value;
}
uint8_t GetSimpleValue (void) const
{
  return m_simpleValue;
}


};


} // namespace ns3

#endif /* BULK_SEND_APPLICATION_H */
