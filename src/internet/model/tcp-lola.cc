#include "tcp-lola.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpLola");
NS_OBJECT_ENSURE_REGISTERED (TcpLola);

TypeId TcpLola::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpLola")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpLola> ()
     ;
  return tid;
}

TcpLola::TcpLola (void) : TcpNewReno (){}
    /*m_alpha (2),
    m_beta (4),
    m_gamma (1),
    m_baseRtt (Time::Max ()),
    m_minRtt (Time::Max ()),
    m_cntRtt (0),
    m_doingVegasNow (true),
    m_begSndNxt (0)
{
  NS_LOG_FUNCTION (this);
}

Need to change this
*/

TcpLola::TcpLola (const TcpLola& sock) : TcpNewReno (sock){}
    /*m_alpha (sock.m_alpha),
    m_beta (sock.m_beta),
    m_gamma (sock.m_gamma),
    m_baseRtt (sock.m_baseRtt),
    m_minRtt (sock.m_minRtt),
    m_cntRtt (sock.m_cntRtt),
    m_doingVegasNow (true),
    m_begSndNxt (0)
{
  NS_LOG_FUNCTION (this);
}

Need to change this
*/

TcpLola::~TcpLola (void)
{
  NS_LOG_FUNCTION (this);
}

std::string TcpLola::GetName () const
{
  return "TcpLola";
}

}
