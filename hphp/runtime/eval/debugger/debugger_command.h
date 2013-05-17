/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010- Facebook, Inc. (http://www.facebook.com)         |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef incl_HPHP_EVAL_DEBUGGER_COMMAND_H_
#define incl_HPHP_EVAL_DEBUGGER_COMMAND_H_

#include "hphp/util/base.h"
#include "hphp/runtime/eval/debugger/debugger_thrift_buffer.h"
#include "hphp/runtime/eval/debugger/debugger_client.h"

namespace HPHP { namespace Eval {
///////////////////////////////////////////////////////////////////////////////
// DebuggerCommand is the base of all commands executed by the debugger. It
// also represents the base binary communication format between DebuggerProxy
// and DebuggerClient.
//
// Each command has serialization logic, plus client- and server-side logic.
// Client-side logic is implemented in the onClient* methods, while server-side
// is in the onServer* methods.
//

DECLARE_BOOST_TYPES(DebuggerCommand);
class DebuggerCommand {
public:
  /**
   * Warning: Do NOT modify exists values, as they are used in binary network
   * protocol, and changing them may create incompatibility between different
   * versions of debugger and server!
   */
  enum Type {
    // Now I sing my A, B, C...
    KindOfAbort               = 1,
    KindOfBreak               = 2,
    KindOfContinue            = 3,
    KindOfDown                = 4,
    KindOfException           = 5,
    KindOfFrame               = 6,
    KindOfGlobal              = 7,
    KindOfHelp                = 8,
    KindOfInfo                = 9,
    KindOfJump_UNUSED         = 10,
    KindOfConstant            = 11,
    KindOfList                = 12,
    KindOfMachine             = 13,
    KindOfNext                = 14,
    KindOfOut                 = 15,
    KindOfPrint               = 16,
    KindOfQuit                = 17,
    KindOfRun                 = 18,
    KindOfStep                = 19,
    KindOfThread              = 20,
    KindOfUp                  = 21,
    KindOfVariable            = 22,
    KindOfWhere               = 23,
    KindOfExtended            = 24,
    KindOfUser                = 25,
    KindOfZend                = 26,
    KindOfComplete            = 27,

    KindOfEval                = 1000,
    KindOfShell               = 1001,
    KindOfMacro               = 1002,
    KindOfConfig              = 1003,
    KindOfInstrument          = 1004,

    // DebuggerProxy -> DebuggerClient
    KindOfInterrupt           = 10000,
    KindOfSignal              = 10001,
  };

  static bool Receive(DebuggerThriftBuffer &thrift, DebuggerCommandPtr &cmd,
                      const char *caller);

public:
  explicit DebuggerCommand(Type type)
    : m_type(type), m_version(0), m_exitInterrupt(false),
      m_incomplete(false) {}

  bool is(Type type) const { return m_type == type;}

  Type getType() const { return m_type;}

  bool send(DebuggerThriftBuffer &thrift);

  bool recv(DebuggerThriftBuffer &thrift);

  // Informs the client of all strings that may follow a break command.
  // Used for auto completion. The client uses the prefix of the argument
  // following the command to narrow down the list displayed to the user.
  virtual void list(DebuggerClient &client);

  // The text to display when the debugger client
  // processes "help <this command name>".
  virtual void help(DebuggerClient &client);

  // Carries out the command, possibly by sending it to the server.
  // If the client is controlled via the API, the setClientOuput method
  // is invoked to update the client with the command output for access
  // via the API.
  void onClient(DebuggerClient &client);

  // Updates the client with information about the execution of this command.
  // This information is not used by the command line client, but can
  // be accessed via the debugger client API exposed to PHP programs.
  virtual void setClientOutput(DebuggerClient &client);

  // Server-side work for a command. Returning false indicates a failure to
  // communicate with the client (for commands that do so).
  virtual bool onServer(DebuggerProxy &proxy);

  // This seems to be confined to eval and print commands.
  // It is not clear that it belongs in this interface or that the
  // assert is safe.
  virtual void handleReply(DebuggerClient &client) { assert(false); }

  // Returns true if DebuggerProxy::processInterrupt() should return
  // to its caller instead of processing further commands from the client.
  bool shouldExitInterrupt() { return m_exitInterrupt;}

  // Returns a non empty error message if the receipt of this command
  // did not complete successfully.
  String getWireError() const { return m_wireError; }

protected:
  // Carries out the command, possibly by sending it to the server.
  virtual void onClientImpl(DebuggerClient &client) = 0;

  // If the first argument of the command is "help" or "?"
  // this displays help text for the command and returns true.
  // Otherwise it returns false.
  bool displayedHelp(DebuggerClient &client);

  // Always called from send and must implement the subclass specific
  // logic for serializing a command to send via Thrift.
  virtual void sendImpl(DebuggerThriftBuffer &thrift);

  // Always called from recv and must implement the subclass specific
  // logic for deserializing a command received via Thrift.
  virtual void recvImpl(DebuggerThriftBuffer &thrift);

  Type m_type;
  std::string m_class; // for CmdExtended
  std::string m_body;
  int m_version;

  bool m_exitInterrupt; // server side breaking out of message loop
  String m_wireError; // used to save temporary error happened on the wire
  bool m_incomplete; // another interrupt comes before the command could finish
};

///////////////////////////////////////////////////////////////////////////////
}}

#endif // incl_HPHP_EVAL_DEBUGGER_COMMAND_H_
