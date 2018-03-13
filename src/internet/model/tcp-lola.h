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
   
    
  //Computes RTT needed to execute Lola algorithm
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);
//Enable/Disable Lola 
 virtual void TcpLola::CongestionStateSet (Ptr<TcpSocketState> tcb,
                              const TcpSocketState::TcpCongState_t newState);




// Get slow start threshold for Lola 
virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);
// Increase congestion window in Cubic manner 
void cubicIncrease();


private:

 
 Time m_queueLow;    //Knob  minimum queue delay
 Time m_queueTarget; //Knob 
 Time m_minRtt;    // minimum value of RTT during meaurement time
 Time m_maxRtt;    // maximum value of RTT during meaurement time
 uint32_t m_cntRtt;                 //!< Number of RTT measurements during last RTT

 

  

};

} // namespace ns3

#endif /* TCP_LOLA_H */
