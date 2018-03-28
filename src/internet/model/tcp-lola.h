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
  
private:
  Time m_queueLow;	//!< Threshold value
  Time m_queueTarget;	//!< Threshold value 
  Time m_queueDelay;	//!< Queuing delay caused by the standing queue
  Time m_syncTime;	//!< During CWnd Hold, the CWnd is unchanged for a fixed amount of time m_syncTime (default value = 250 ms).
  Time m_nowRtt;
  Time m_minRtt;	//!< Minimum value of RTT during measurement time
  Time m_maxRtt;	//!< Maximum value of RTT during measurement time
  double m_factorC;  	//!< Unit-less factor (m_factorC = 0.4)
  Time m_timeSinceRedn;	//!< Time since last CWnd reduction
  double m_factorK;	//!< Recalculated whenever CWnd has to be reduced
  uint32_t m_cwndMax;	//!< Size of CWnd before last reduction
  uint32_t m_cntRtt;	//!< Number of RTT measurements during last RTT
};

} // namespace ns3

#endif /* TCP_LOLA_H */
