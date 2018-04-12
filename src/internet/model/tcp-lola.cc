#include "tcp-lola.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"
#include "ns3/simulator.h"
#include <math.h>
#include <float.h>

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
  m_minRtt=Seconds(DBL_MAX);	
  NS_LOG_FUNCTION (this);
}

TcpLola::TcpLola (const TcpLola& sock) :TcpNewReno (sock)
{
  m_minRtt=Seconds(DBL_MAX);
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

TcpLola::~TcpLola (void)
{
  NS_LOG_FUNCTION (this);
}

void TcpLola::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
  if (rtt.IsZero ())
    {
      return;
    }
	
  //m_minRtt = tcb->m_minRtt;
  m_minRtt=std::min(m_minRtt,rtt);
  m_maxRtt = std::max (m_maxRtt, rtt);
  m_curRtt = rtt;
  m_queueDelay = m_curRtt - m_minRtt;
  //NS_LOG_INFO("----------"<<m_minRtt<<"----------"<<m_maxRtt<<"----------"<<m_curRtt<<"----------"<<m_queueDelay);
}

void TcpLola::CongestionStateSet (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
{ 
  if (newState == TcpSocketState::CA_CWR || newState == TcpSocketState::CA_RECOVERY || newState == TcpSocketState::CA_LOSS )
    {
     m_cwndRednTimeStamp = Simulator::Now (); 
     m_state  = 1;
     
     m_flag = 0;
     
     //NS_LOG_INFO("----------"<<newState);
    }
}

void TcpLola::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  if (m_state == 0)
  {
  	m_cwndMaxTemp = std::max (static_cast<uint32_t>(tcb->m_cWnd), m_cwndMaxTemp);
  }
  if (m_state == 1)
  {
  	m_cwndMax = m_cwndMaxTemp;
  	m_state = 0;
  	m_cwndMaxTemp = 0;
  }

  if (m_maxRtt - m_minRtt > 2 * m_queueLow && m_flag==1 )	 
  {
    NS_LOG_FUNCTION("-----------Cubic Increase-----------");
    double x;
    x = m_factorC * std::pow ((Simulator::Now ().GetSeconds() - m_cwndRednTimeStamp.GetSeconds() - m_factorK), 3.0) + static_cast<double> (m_cwndMax);	
    tcb->m_cWnd = static_cast<uint32_t> (x * tcb->m_segmentSize); 
    m_flag = 2;
    
  }
  
  else if (m_queueDelay > m_queueLow && m_flag==2 )
  {
    uint32_t X;
    NS_LOG_FUNCTION("-----------Fair Flow Balancing-----------");
    if (m_fair == 0)
    {
     X = 0;
     m_fair = 1;   	
     m_fairFlowTimeStamp = Simulator::Now ();
    }
    else
    {
      X = pow((static_cast<uint32_t> (Simulator::Now ().GetSeconds ()) - m_fairFlowTimeStamp.GetSeconds ())/m_phi, 3);
    }
    
    m_qData = (tcb->m_cWnd * m_queueDelay) / m_curRtt;   
    if(m_qData < X)
    {
      tcb->m_cWnd += X - m_qData;
    }
    m_flag = 3;  
    
  }

  else if (m_queueDelay	 > m_queueTarget && m_flag == 3)
  { 
    NS_LOG_FUNCTION("-----------CWnd Hold-----------");
    
     //NS_LOG_INFO("----Time now --:"<<Simulator::Now()<<"----");
     //m_expiredEvent = Simulator::Schedule (MilliSeconds (250), &TcpLola::TailoredDecrease, this);
      m_cwndHoldTime=m_syncTime.GetMilliSeconds();
      TimerHandler();
     //sleep(250);
     //NS_LOG_INFO("----Time now --:"<<Simulator::Now()<<"----");



     m_qData = tcb->m_cWnd * m_queueDelay/m_curRtt; 
     updateKfactor();
     tcb->m_cWnd = (tcb->m_cWnd - m_qData) * m_gamma;
     m_state = 1;
     m_flag = 1;
     
     if(++m_minRttResetCounter==100){
     	m_minRtt=Seconds(DBL_MAX);
     }
  }

  else if(m_flag == 0)
  {	
    NS_LOG_FUNCTION("-----------Slow Start-----------");
    TcpNewReno::SlowStart (tcb, segmentsAcked);
    m_state = 0;
    m_flag = 1;
   
  }
  m_cwndMaxTemp = std::max (static_cast<uint32_t>(tcb->m_cWnd), m_cwndMaxTemp);
}

std::string TcpLola::GetName () const
{
  return "TcpLola";
}

 uint32_t TcpLola::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
 {
   return std::max (std::min (tcb->m_ssThresh.Get (), tcb->m_cWnd.Get () - tcb->m_segmentSize), 2 * tcb->m_segmentSize);
 }

void TcpLola::updateKfactor()
{
  double temp = (m_curRtt - m_minRtt * m_gamma) * m_cwndMax / m_curRtt ;
  temp = temp / m_factorC;
  m_factorK = cbrt(temp);
}

void TcpLola::TimerHandler()
{ 
  if (m_cwndHoldTime > 0)
  {
    m_cwndHoldTime--;
	m_cWndHoldEvent = Simulator::Schedule (MilliSeconds (1), &TcpLola::TimerHandler, this);
  }
}
void TcpLola::TailoredDecrease(){
  //m_qData = tcb->m_cWnd * m_queueDelay/m_curRtt; 
    updateKfactor();
    // tcb->m_cWnd = (tcb->m_cWnd - m_qData) * m_gamma;
    m_state = 1;
    //m_flag=1;
  }
} // namespace ns3
