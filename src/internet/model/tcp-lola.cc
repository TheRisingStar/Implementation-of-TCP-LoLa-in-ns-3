#include "tcp-lola.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"
#include "ns3/simulator.h"
#include <math.h>

NS_LOG_COMPONENT_DEFINE ("TcpLola");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpLola);

TypeId TcpLola::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::TcpLola")
    .SetParent<TcpNewReno>()
    .SetGroupName ("Internet")
    .AddConstructor<TcpLola>()
    .AddAttribute ("C", "Unit-less factor",
                   DoubleValue (0.4),
                   MakeDoubleAccessor (&TcpLola::m_factorC),
                   MakeDoubleChecker <double> (0.0))
    .AddAttribute ("Phi", "Fair flow balancing curve factor",
                   UintegerValue (35),
                   MakeUintegerAccessor (&TcpLola::m_phi),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Gamma", "Factor to update K",
                   DoubleValue (973/1024),
                   MakeDoubleAccessor (&TcpLola::m_gamma),
                   MakeDoubleChecker <double> (0.0))
    .AddAttribute ("SyncTime","CWnd hold time in ms",
                   TimeValue (MilliSeconds (250)),
                   MakeTimeAccessor (&TcpLola::m_syncTime),
                   MakeTimeChecker ())     
    .AddAttribute ("QueueLow","Minimum queue delay expected in ms",
                   TimeValue (MilliSeconds (1)),
                   MakeTimeAccessor (&TcpLola::m_queueLow),
                   MakeTimeChecker ())          
    .AddAttribute ("QueueTarget","Target queue delay in ms",
                   TimeValue (MilliSeconds (5)),
                   MakeTimeAccessor (&TcpLola::m_queueTarget),
                   MakeTimeChecker ())     
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
  NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps> TcpLola::Fork (void)
{
  return CopyObject<TcpLola> (this);
}

void TcpLola::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
  //NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);

  if (rtt.IsZero ())
    {
      return;
    }
	
  m_minRtt = tcb->m_minRtt ;
  // NS_LOG_DEBUG ("Updated m_minRtt = " << m_minRtt);

  m_maxRtt = std::max (m_maxRtt, rtt);
  // NS_LOG_DEBUG ("Updated m_maxRtt = " << m_maxRtt);
  
  m_curRtt = rtt;
  // NS_LOG_DEBUG ("Updated m_curRtt = " << m_curRtt);
  
  m_queueDelay = m_curRtt - m_minRtt;
  // NS_LOG_DEBUG ("Updated m_queueDelay = " << m_queueDelay);
 
}

void TcpLola::CongestionStateSet (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
{
     // NS_LOG_FUNCTION (this << tcb << newState);
  
  if (newState == TcpSocketState::CA_CWR || newState == TcpSocketState::CA_RECOVERY || newState == TcpSocketState::CA_LOSS )
    {
      updateKfactor();
      m_cwndRednTimeStamp = Simulator::Now (); 
    }
}

void TcpLola::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  //NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  //NS_LOG_INFO("---------"<<m_queueDelay<<"-----------"<<m_minRtt<<"------"<<m_curRtt<<"--------"<<m_maxRtt<<"----------");
  //NS_LOG_INFO("------"<<(tcb->m_nextTxSequence - tcb->m_lastAckedSeq - 1)<<"---------"<<tcb->m_cWnd<<"----------"<<tcb->m_segmentSize<<"----");
  if (m_maxRtt - m_minRtt > 2 * m_queueLow && (m_flag==1))	 
  {
    m_flag=2;
    NS_LOG_FUNCTION("----------------------Cubic Increase");

    double x;
    x = m_factorC * std::pow ((Simulator::Now ().GetSeconds() - m_cwndRednTimeStamp.GetSeconds() - m_factorK), 3.0) + static_cast<double> (m_cwndMax);	
    tcb->m_cWnd = static_cast<uint32_t> (x * tcb->m_segmentSize); 
  }
  
  else if (m_queueDelay > m_queueLow && (m_flag==2 ))
  {
  	m_flag=3;
    NS_LOG_FUNCTION("------------Fair Flow Balancing");
    uint32_t X = pow((static_cast<uint32_t> (Simulator::Now ().GetSeconds ()) - m_fairFlowTimeStamp.GetSeconds ())/m_phi, 3);  
   //NS_LOG_FUNCTION("------------Fair Flow Balancing-- time diff calc ");
    m_qData = (tcb->m_nextTxSequence - tcb->m_lastAckedSeq - 1) * tcb->m_segmentSize;  
    if(m_qData < X)
    {
      tcb->m_cWnd = tcb->m_cWnd + X - m_qData;
    }  
    // NS_LOG_FUNCTION("------------Fair Flow Balancing --- end----"<<m_queueDelay<<"-----"<<m_queueTarget<<"--");  
  }
  else if (m_queueDelay	 > m_queueTarget && (m_flag==3))
  { 
    m_flag=1;
    NS_LOG_FUNCTION("--------------------------CWnd Hold");  
    m_fairFlowTimeStamp = Simulator::Now ();
    NS_LOG_INFO( "-----------;--------"<<Simulator::Now ()<<"---------------");
     //Timer a;
    //a.SetDelay(m_syncTime);
    //a.Schedule(m_syncTime);  
    NS_LOG_INFO( "-----------;--------"<<Simulator::Now ()<<"---------------");
    
    m_qData = (tcb->m_nextTxSequence - tcb->m_lastAckedSeq - 1) * tcb->m_segmentSize; 
   
    m_cwndMax = tcb->m_cWnd;
    // NS_LOG_DEBUG (this << "Updated m_cWndmax = " << m_cwndMax);
  
    tcb->m_cWnd = (tcb->m_cWnd - m_qData) * m_gamma;
    
  }
  
  else if(m_flag==0)
  {	m_flag=1;
    NS_LOG_FUNCTION("------------------Slow start");
    TcpNewReno::SlowStart (tcb, segmentsAcked);
  }
}

std::string TcpLola::GetName () const
{
  return "TcpLola";
}

uint32_t TcpLola::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
 // NS_LOG_FUNCTION (this << tcb << bytesInFlight);
  return std::max (std::min (tcb->m_ssThresh.Get (), tcb->m_cWnd.Get () - tcb->m_segmentSize), 2 * tcb->m_segmentSize);
}

void TcpLola::updateKfactor()
{
 //NS_LOG_FUNCTION (this) << "-------Update K factor");
  double temp = (m_curRtt - m_minRtt * m_gamma) * m_cwndMax * m_factorC / m_curRtt;
  m_factorK = cbrt(temp);
}
/*
void TcpLola::TimerHandler()
{ 
  NS_LOG_FUNCTION (this);
  if (m_tempTime > 0)
  {
    m_tempTime--;
	m_expiredEvent = Simulator::Schedule (Seconds (1), &TcpLola::TimerHandler, this);
  }
}*/
} // namespace ns3
