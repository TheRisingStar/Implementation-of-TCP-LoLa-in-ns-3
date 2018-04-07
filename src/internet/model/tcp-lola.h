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
     
  /**
   * \brief Compute RTTs needed to execute Vegas algorithm
   *
   * The function filters RTT samples from the last RTT to find
   * the current smallest propagation delay + queueing delay (minRtt).
   * We take the minimum to avoid the effects of delayed ACKs.
   *
   * The function also min-filters all RTT measurements seen to find the
   * propagation delay (baseRtt).
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   * \param rtt last RTT
   *
   */
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);

  /**
   * \brief Enable/disable Vegas algorithm depending on the congestion state
   *
   * We only start a Vegas cycle when we are in normal congestion state (CA_OPEN state).
   *
   * \param tcb internal congestion state
   * \param newState new congestion state to which the TCP is going to switch
   */
  virtual void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);

  uint32_t GetTarget(uint32_t time);

  /**
   * \brief Adjust cwnd following Vegas linear increase/decrease algorithm
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   */
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  /**
   * \brief Get slow start threshold following Vegas principle
   *
   * \param tcb internal congestion state
   * \param bytesInFlight bytes in flight
   *
   * \return the slow start threshold value
   */
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);

  virtual Ptr<TcpCongestionOps> Fork ();
  
  void updateKfactor();
  void TimerHandler();
  
private:
  Time m_queueLow;	    //!< Threshold value
  Time m_queueTarget;	//!< Threshold value 
  Time m_queueDelay;	    //!< Queuing delay caused by the standing queue
  
  Time m_syncTime;	    //!< During CWnd Hold, the CWnd is unchanged for a fixed amount of time m_syncTime
  //uint32_t m_tempTime;
  
  Time m_curRtt;			//!< Current value of RTT
  Time m_minRtt;	        //!< Minimum value of RTT during measurement time
  Time m_maxRtt;	        //!< Maximum value of RTT during measurement time
  
  Time m_cwndRednTimeStamp;	//!< Time stamp when CWnd is reduced
  Time m_fairFlowTimeStamp; //!< Time stamp when Queue Target is exceded
  
  double m_factorC;      	//!< Unit-less factor
  double m_factorK;	        //!< Recalculated whenever CWnd has to be reduced
  
  uint32_t m_cwndMax;   	//!< Size of CWnd before last reduction
  uint32_t m_qData;	    	//!< Amount of data the flow itself has queued at the bottleneck
  
  uint32_t m_phi;	    	//!< Fair flow balancing curve factor
  double m_gamma;   
  
  uint32_t m_flag;        
  
  EventId m_expiredEvent;
};

} // namespace ns3

#endif /* TCP_LOLA_H */
