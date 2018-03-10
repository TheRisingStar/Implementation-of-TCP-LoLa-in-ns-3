#include "tcp-lola.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"

NS_LOG_COMPONENT_DEFINE ("TcpLola");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpLola);

TypeId TcpLola::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::TcpLola")
    .SetParent<TcpNewReno>()
    .SetGroupName ("Internet")
    .AddConstructor<TcpLola>()
  ;
  return tid;
}

TcpLola::TcpLola (void) :TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
}

TcpLola::TcpLola (const TcpLola& sock) :TcpNewReno (sock)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

TcpLola::~TcpLola (void)
{
}

} // namespace ns3
