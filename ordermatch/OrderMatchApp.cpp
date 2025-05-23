#include "OrderMatchApp.h"
#include <quickfix/Session.h>
#include <stdexcept>
#include <iostream>


void OrderMatchApp::onMessage(const FIX42::NewOrderSingle& msg,
                              const FIX::SessionID&) {

  FIX::SenderCompID sender; msg.getHeader().get(sender);
  FIX::TargetCompID target; msg.getHeader().get(target);

  FIX::ClOrdID   clOrdID;   msg.get(clOrdID);
  FIX::Symbol    symbol;    msg.get(symbol);
  FIX::Side      side;      msg.get(side);
  FIX::OrdType   ordType;   msg.get(ordType);
  FIX::Price     price(0);
  if(ordType == FIX::OrdType_LIMIT) msg.get(price);
  FIX::OrderQty  qty;       msg.get(qty);
  FIX::TimeInForce tif(FIX::TimeInForce_DAY);
  msg.getFieldIfSet(tif);

  try {
    if(tif != FIX::TimeInForce_DAY)
      throw std::logic_error("Unsupported TIF, use Day");

    Order order(clOrdID, symbol,
                sender, target,
                convert(side), convert(ordType),
                price, (long)qty);

    processOrder(order);
  }
  catch(std::exception& e){
    rejectOrder(sender, target, clOrdID, symbol, side, e.what());
  }
}

void OrderMatchApp::onMessage(const FIX42::OrderCancelRequest& msg,
                              const FIX::SessionID&) {
  FIX::OrigClOrdID orig; msg.get(orig);
  FIX::Symbol sym; msg.get(sym);
  FIX::Side  side; msg.get(side);

  try { processCancel(orig, sym, convert(side)); }
  catch(std::exception&) {}
}

void OrderMatchApp::onMessage(const FIX42::MarketDataRequest& msg,
                              const FIX::SessionID&) {
  FIX::SubscriptionRequestType sub; msg.get(sub);
  if(sub != FIX::SubscriptionRequestType_SNAPSHOT)
    throw FIX::IncorrectTagValue(sub.getTag());
}

void OrderMatchApp::updateOrder(const Order& o, char ordStatus) {
  FIX::TargetCompID tgt(o.getOwner());
  FIX::SenderCompID snd(o.getTarget());

  FIX42::ExecutionReport rpt(
      FIX::OrderID(o.getClientID()),
      FIX::ExecID(idGen_.genExecutionID()),
      FIX::ExecTransType(FIX::ExecTransType_NEW),
      FIX::ExecType(ordStatus),
      FIX::OrdStatus(ordStatus),
      FIX::Symbol(o.getSymbol()),
      convert(o.getSide()),
      FIX::LeavesQty(o.getOpenQuantity()),
      FIX::CumQty(o.getExecutedQuantity()),
      FIX::AvgPx(o.getAvgExecutedPrice()));

  rpt.set(FIX::ClOrdID(o.getClientID()));
  rpt.set(FIX::OrderQty(o.getQuantity()));

  if(ordStatus==FIX::OrdStatus_FILLED
     || ordStatus==FIX::OrdStatus_PARTIALLY_FILLED){
    rpt.set(FIX::LastShares(o.getLastExecutedQuantity()));
    rpt.set(FIX::LastPx(o.getLastExecutedPrice()));
  }

  std::lock_guard<std::mutex> g(m_);
  try { FIX::Session::sendToTarget(rpt, snd, tgt); }
  catch(FIX::SessionNotFound&) {}
  execCb_(rpt);               
}

void OrderMatchApp::rejectOrder(
    const FIX::SenderCompID& sender,
    const FIX::TargetCompID& target,
    const FIX::ClOrdID& clOrdID,
    const FIX::Symbol& symbol,
    const FIX::Side& side,
    const std::string& why) {
  FIX::TargetCompID tgt(sender.getValue());
  FIX::SenderCompID snd(target.getValue());

  FIX42::ExecutionReport rpt(
      FIX::OrderID(clOrdID.getValue()),
      FIX::ExecID(idGen_.genExecutionID()),
      FIX::ExecTransType(FIX::ExecTransType_NEW),
      FIX::ExecType(FIX::ExecType_REJECTED),
      FIX::OrdStatus(FIX::ExecType_REJECTED),
      symbol,
      side,
      FIX::LeavesQty(0),
      FIX::CumQty(0),
      FIX::AvgPx(0));
  rpt.set(clOrdID);
  rpt.set(FIX::Text(why));

  try { FIX::Session::sendToTarget(rpt, snd, tgt); }
  catch(FIX::SessionNotFound&) {}
  execCb_(rpt);
}

void OrderMatchApp::processOrder(const Order& order){
  if(matcher_.insert(order)){
    acceptOrder(order);
    std::queue<Order> fills;
    matcher_.match(order.getSymbol(), fills);
    while(!fills.empty()){
      fillOrder(fills.front());
      fills.pop();
    }
  } else rejectOrder(order);
}
void OrderMatchApp::processCancel(const std::string& id,
                                  const std::string& sym,
                                  Order::Side side){
  Order& o = matcher_.find(sym, side, id);
  o.cancel();
  cancelOrder(o);
  matcher_.erase(o);
}

Order::Side OrderMatchApp::convert(const FIX::Side& s){
  if(s == FIX::Side_BUY)  return Order::buy;
  if(s == FIX::Side_SELL) return Order::sell;
  throw std::logic_error("Unsupported Side");
}


Order::Type OrderMatchApp::convert(const FIX::OrdType& t){
  if(t == FIX::OrdType_LIMIT)  return Order::limit;
  if(t == FIX::OrdType_MARKET) return Order::market;
  throw std::logic_error("Unsupported OrdType - use MARKET or LIMIT");
  }

FIX::OrdType OrderMatchApp::convert(Order::Type t){
  return FIX::OrdType(t==Order::limit ? FIX::OrdType_LIMIT
                                      : FIX::OrdType_MARKET);
    }

FIX::Side OrderMatchApp::convert(Order::Side s){
  return s==Order::buy ? FIX::Side(FIX::Side_BUY)
                       : FIX::Side(FIX::Side_SELL);
}\

void OrderMatchApp::rejectOrder(const Order& o) {
  updateOrder(o, FIX::OrdStatus_REJECTED);
}

