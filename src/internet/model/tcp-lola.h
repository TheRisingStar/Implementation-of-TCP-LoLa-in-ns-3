#ifndef TCP_LOLA_H
#define TCP_LOLA_H

#include "tcp-congestion-ops.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-value.h"

namespace ns3 {

class Packet;
class TcpHeader;
class Time;
class EventId;

class TcpLola : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpLola (void);
  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  TcpLola (const TcpLola& sock);
  virtual ~TcpLola (void);

  
  



private:
 

protected:
  

};

} // namespace ns3

#endif /* TCP_LOLA_H */
