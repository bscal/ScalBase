#pragma once

#include "Core.h"
#include "StaticArray.h"

enum class EventResult
{
    Ok,
    Fail,
    Cancel,
};

enum class Events
{
    AppStart,

    MaxEvents
};

enum EventPriorties
{
    EVENT_PRIO_HIGHEST,
    EVENT_PRIO_HIGH,
    EVENT_PRIO_NORMAL,
    EVENT_PRIO_LOW,
    EVENT_PRIO_LOWEST,

    EVENT_PRIO_MAX
};

typedef EventResult EventCallback(void* event, void* stack);

constant_var size_t EVENT_MAX_LISTENERS = 64;

struct Listener
{
    EventCallback Callback;
    void* UserData;
};

struct Event
{
    StaticArray<StaticList<Listener, EVENT_MAX_LISTENERS>, EVENT_PRIO_MAX> Priorieties;
};

struct EventManager
{
    StaticArray<Event, (size_t)Events::MaxEvents> Events;
};

global_var EventManager g_EventManager;

EventResult EventsTrigger(Events eventType, void* eventData)
{
    Event* event = g_EventManager.Events.At((size_t)eventType);
    SAssert(event);

    EventResult result;

    for (size_t prio = 0; 0 < EVENT_PRIO_MAX; ++prio)
    {
        for (size_t listenerIdx = 0; listenerIdx < event->Priorieties.At(prio)->Count; ++listenerIdx)
        {
            Listener* listener = event->Priorieties.At(prio)->At(listenerIdx);
            SAssert(listener);
            SAssert(listener->Callback);

            result = listener->Callback(eventData, listener->UserData);
        }
    }

    return result;
}

void EventsRegisterListener(Events eventType, EventPriorties prio, Listener* listener)
{
    Event* event = g_EventManager.Events.At((size_t)eventType);
    SAssert(event);

    int numOfListeners = event->Priorieties.At(prio)->Count;
    if (numOfListeners < EVENT_MAX_LISTENERS)
    {
        event->Priorieties.At(prio)->Push(listener);
    }
}

void EventsRegisterListener(Events eventType, EventPriorties prio, Listener* listener)
{
    Event* event = g_EventManager.Events.At((size_t)eventType);
    SAssert(event);

    int numOfListeners = event->Priorieties.At(prio)->Count;
    if (numOfListeners < EVENT_MAX_LISTENERS)
    {
        event->Priorieties.At(prio)->Push(listener);
    }
}
