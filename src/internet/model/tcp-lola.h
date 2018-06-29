#ifndef TCP_LOLA_H
#define TCP_LOLA_H

#include "tcp-congestion-ops.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-value.h"
#include "ns3/event-id.h"

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

  virtual std::string GetName () const;


  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);


  virtual void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);

  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  enum lolaStates
  {
    NS_SLOW_START,
    NS_CUBIC,
    NS_FAIR_FLOW,
    NS_CWND_HOLD,
    NS_TAIL_DECREASE
  };

private:
  void callSlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  void callCubic (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  void callFairFlow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  void callCwndHold (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  void callTailDecrease (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  void updateKfactor ();

  //void updateRttList ();   //List push method to measure curRtt

  Time m_queueLow;              //!< Threshold value
  Time m_queueTarget;            //!< Threshold value
  Time m_queueDelay;            //!< Queuing delay caused by the standing queue
  Time m_syncTime;              //!< During CWnd Hold, the CWnd is unchanged for a fixed amount of time m_syncTime

  uint32_t m_phi;               //!< Fair flow balancing curve factor
  double m_gamma;
  double m_factorC;             //!< Unit-less factor


  double m_factorK;             //!< Recalculated whenever CWnd has to be reduced

  Time m_curRtt;                        //!< Current value of RTT
  Time m_minRtt;                //!< Minimum value of RTT during measurement time
  Time m_maxRtt;                //!< Maximum value of RTT during measurement time
  uint32_t m_qData;             //!< Amount of data the flow itself has queued at the bottleneck
  uint32_t m_minRttResetCounter;

  Time m_measureTime;           //!<Measure Time Interval to caculate current RTT value
  Time m_cwndHoldTime;

  Time m_measureTimeStamp;
  Time m_cwndRednTimeStamp;     //!< Time stamp when CWnd is reduced
  Time m_fairFlowTimeStamp;     //!< Time stamp when Queue Target is exceded

  uint32_t m_cwndMax;           //!< Size of CWnd before last reduction
  uint32_t m_cwndMaxTemp;

  uint32_t m_nextState;

  bool m_cwndReduced   {
    false
  };
  bool m_fairFlowStart {
    false
  };
  bool m_cwndHoldStart {
    false
  };

  /* flag method to measure curRtt
  bool m_measureTimeStart {
    false
  };
  */

  /*List push method to measure curRtt
  typedef std::list< std::pair<Time, Time> >  RttList;
  typedef std::list< std::pair<Time, Time> >::iterator  RttListIter;
  RttList m_rttList;
  EventId m_expiredEvent;                //!< The Event to trigger updateRttList
  */
};

} // namespace ns3

#endif /* TCP_LOLA_H */
