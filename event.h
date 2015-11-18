#pragma once
#include <functional>
#include <thread>
#include <future>
#include <mutex>
#include <queue>

namespace raven
{
namespace event
{

/** @brief A queue of delayed events

The basic ideas follow boost::asio

 - no boost libraries needed, everything is standard C++11

 - simple run function on timer

 - No nework i/o.  This should be built on top of this.

*/
class cEventQueue
{
public:

    /** Event handler type

    It is a std::function for a function returning void and taking no parameters.
    All event handlers need to be like this.

    */

    typedef std::function< void() > handler_t;

    /** Post an event handler to be run as soon as possible

    @param[in] f the event handler

    This can be called from any thread
    ( the queue is protected by a mutex )
    but all the handler will be invoked on by one
    from the same thread
    as called raven::event::theEventQueue.Run()

    */
    void Post( handler_t f )
    {
        std::lock_guard<std::mutex> lock(myMutex);
        myQ.push( f );
    }

    /** Run the event handlers in the order they were posted

    This will never return
    It will keep running ( even if there is no work )
    until Stop() is called from one of the event handlers

    */

    void Run()
    {
        // keep on running
        while( 1 )
        {
            // return if stop flag set
            if( myStopFlag )
                break;

            // get copy of next handler if one waiting
            if( get_next_handler() )
            {
                /* execute handler

                This is where all the application code is executed
                in this thread one by one
                ( even if they were posted 'simultaineously' )

                */

                myHandlerCopy();
            }

            // yield to other threads if no event handlers waiting
            yield();
        }
    }

    /**  get copy of next handler if one waiting

        @return true if event handler waiting

        A copy of the handler will be made in the attribute myHandlerCopy

    */

    bool get_next_handler()
    {
        // prevent access to queue by other threads
        std::lock_guard<std::mutex> lock(myMutex);

        // do nothing if no handler waiting
        if( myQ.empty() )
            return false;

        // move handler from queue to attribute copy
        myHandlerCopy = myQ.front();
        myQ.pop();

        return true;

    }
    /** yield to other threads if no event handlers waiting */
    void yield()
    {
        // prevent access to queue by other threads
        std::lock_guard<std::mutex> lock(myMutex);

        // yield to other threads if no handlers waiting
        if( myQ.empty() )
            std::this_thread::sleep_for (
                std::chrono::milliseconds( 1 ));

    }
    /**  Stop the event handler running

    This should be called from an event handler

    */

    void Stop()
    {
        myStopFlag = true;
    }

private:
    bool myStopFlag;                    ///< true if stop requested
    handler_t myHandlerCopy;            ///< currently running event handler
    std::queue< handler_t > myQ;        ///< the queue of event handler ready to run
    std::mutex myMutex;                 ///< prevents contention between threads trying to access the handler queue
};

//  Construct the one and only event queue instance

cEventQueue theEventQueue;

/**  @brief A base timer to invoke a function at some later date

    This is an abstract base class.  To use it must be specialized
    and the function handle_timer_event must be over-ridden

    e.g.

<pre>

class cTimedFunction : public raven::event::cTimer
{
public:
    void handle_timer_event()
    {
        // do something useful
    }
};

cTimedFunction tf;
tf.WaitThenPost( 1000 );    // do something useful 1 second from now

</pre>
*/

class cTimer
{
public:

    /** Non-blocking delayed run of handle_timer_event()

    @param[in] msec  how long to wait before running handler

    This returns immediatly.  After the delay is completed
    ( the delay uses a sleeping thread, so consumes almost no CPU )
    the function will be posted to the event queue where it will be run
    as soon as possible after any other posted event handlers

    */
    void WaitThenPost( int msecs )
    {
        WaitThenPost(
            msecs,
            cEventQueue::handler_t( std::bind(&cTimer::handle_timer_event, this) ) );
    }

    /** Over-ride this with the code you want to execute
    when the timer expires.

    Even if you are not going to use this, it must be overridden,
    like this

    void handle_timer_event() {}

    */

    virtual void handle_timer_event() = 0;

    /** Non-blocking delayed run of a supplied function

    @param[in] msec  how long to wait before running handler
    @param[in] f supplied event handler

    The supplied function must return void, but can use parameters

    e.g.

    <pre>
        WaitThenPost(
            msecs,
            cEventQueue::handler_t( std::bind(&myTimer::myFunction, this, param1, param2 ) ) );
    </pre>

    */

    void WaitThenPost( int msecs, cEventQueue::handler_t f )
    {
        // run blocking wait and post in a sleeping thread

        /*

        Note that we store the future that is returned in an attribute
        even though we do not use it for antything

        The thread will not start if the return is 'disregarded'
        i.e. if the return is destroyed.  We must keep it in scope
        to keep the thread running

        */
        myfuture = std::async(
                       std::launch::async,              // insist on starting immediatly
                       &cTimer::BlockWaitThenPost,      // run the blocking wait and post
                       this,                            // for this instance
                       msecs,                           // length to delay
                       f );                             // function to post when the delay completes
    }

private:

    /**  Blocking wait and post */
    void BlockWaitThenPost( int msec, cEventQueue::handler_t f )
    {
        // sleep for reuired delay
        std::this_thread::sleep_for (
            std::chrono::milliseconds( msec ));

        // post the event handler to the event queue
        // ( It will run in the main thread,
        //  not this one which now dies )

        theEventQueue.Post( f );
    }

    std::future< void > myfuture;       ///< this keeps the running sleeping thread in scope
};

/** Stop the event handler after a delay */

class cTimedStop : public cTimer
{
public:

    /** ctor

    @param[in] secs to wait, then stop the event handler

    Construct this before the call the theEventQueue.Run()
    to limit the run to the specified period.

    */

    cTimedStop( int secs )
    {
        WaitThenPost( 1000 * secs );
    }

    /**  event handler to stop them all */

    void handle_timer_event()
    {
        theEventQueue.Stop();
    }
};

}
}

