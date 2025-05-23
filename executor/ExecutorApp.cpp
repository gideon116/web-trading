#include "ExecutorApp.h"
#include <quickfix/Session.h>
#include <stdexcept>

void ExecutorApp::onMessage(const FIX42::NewOrderSingle& msg,
                            const FIX::SessionID&) {
  FIX::OrdType ordType;  msg.get(ordType);
    if (ordType != FIX::OrdType_LIMIT && ordType != FIX::OrdType_MARKET)
    throw FIX::IncorrectTagValue(ordType.getTag());

  FIX::Symbol   symbol;    msg.get(symbol);
  FIX::Side     side;      msg.get(side);
  FIX::OrderQty qty;       msg.get(qty);
  FIX::Price price(0); if (ordType == FIX::OrdType_LIMIT)  msg.get(price);
  FIX::ClOrdID  clID;      msg.get(clID);
  FIX::Account  acct;
  bool hasAcct = msg.isSet(acct);

  FIX42::ExecutionReport rpt(
      FIX::OrderID(genOrderID()),
      FIX::ExecID (genExecID()),
      FIX::ExecTransType(FIX::ExecTransType_NEW),
      FIX::ExecType(FIX::ExecType_FILL),
      FIX::OrdStatus(FIX::OrdStatus_FILLED),
      symbol,
      side,
      FIX::LeavesQty(0),
      FIX::CumQty(qty),
      FIX::AvgPx(price));
  rpt.set(clID);
  rpt.set(qty);
  rpt.set(FIX::LastShares(qty));
  rpt.set(FIX::LastPx(price));
  if(hasAcct) rpt.setField(msg.get(acct));

  {
    std::lock_guard<std::mutex> g(m_);
    FIX::Session::sendToTarget(rpt, session_);
  }
  execCb_(rpt);
}
