#ifndef TCPLOLA_H
#define TCPLOLA_H


#include "ns3/tcp-congestion-ops.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-value.h"
#include "ns3/tcp-scalable.h"

namespace ns3 {


class TcpLola : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Create an unbound tcp socket.
   */
  TcpLola (void);

  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  TcpLola (const TcpLola& sock);
  virtual ~TcpLola (void);

  virtual std::string GetName () const;


protected:
private:
};

} // namespace ns3

#endif // TCPVEGAS_H
