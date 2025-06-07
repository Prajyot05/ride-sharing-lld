1. High‐Level Architecture
    DispatchService (Singleton): Core orchestrator. Maintains in‐memory lists of drivers and rides. Handles ride requests, status updates, and completion.
    User Hierarchy: Abstract User → Rider & Driver.
    Vehicle & VehicleType: Encapsulate vehicle details (type, per‐km fare, capacity).
    Location: Encapsulates latitude/longitude with a method to compute Euclidean distance.
    Ride & RideRequest: Represent ride details, status, assigned driver, observers, distance, fare.
    MatchingStrategy (Strategy): Interface for driver selection logic. Implemented by NearestDriverStrategy and BestRatedDriverStrategy. Easily extended.
    FareCalculator (Decorator): Base class BaseFareCalculator computes core fare. Decorators (SurgePricingDecorator, DiscountDecorator) wrap the base to modify final fare.
    PaymentProcessor: Abstracts payment processing. DummyPaymentProcessor used in prototype.
    Observer Pattern: RideObserver interface with RiderNotificationService and DriverNotificationService as concrete observers. Ride maintains a list of observers, calls them on status changes.

2. SOLID Principles
    Single Responsibility (S):
        DispatchService only manages drivers/rides and orchestrates flows.
        Driver & Rider only hold user‐specific data and minimal methods to update location or request a ride.
        Ride only manages ride state, observer notifications, and keeps ride‐specific data.
        BaseFareCalculator only calculates fare; decorators handle optional extensions.

    Open/Closed (O):
        To add a new driver matching strategy (e.g., “Most Efficient Driver”), just implement MatchingStrategy—no change to DispatchService.
        To introduce “peak‐hour surcharge,” add a new SurgePricingDecorator without touching BaseFareCalculator.
        To add a new notification channel (e.g., SMS, email), implement RideObserver and attach to rides.

    Liskov Substitution (L):
        Every MatchingStrategy implementation can replace the interface.
        Every FareCalculator (with any decorator chain) conforms to FareCalculator interface.

    Interface Segregation (I):
        We use small interfaces (MatchingStrategy, FareCalculator, PaymentProcessor, RideObserver) so clients implement only what they need.

    Dependency Inversion (D):
        DispatchService depends on the abstractions (MatchingStrategy, FareCalculator, PaymentProcessor), not on concrete classes. At runtime, we inject the concrete implementations.

3. Design Patterns Employed
    Singleton (DispatchService):
        Ensures only one dispatch orchestrator manages all drivers and rides.

    Factory (RideFactory):
        Encapsulates the creation of Ride objects. If new ride subtypes are needed (e.g., CarpoolRide), modify only RideFactory.

    Strategy (MatchingStrategy + implementations):
        Abstracts driver matching logic. Swappable at runtime.

    Observer (RideObserver + Ride):
        Decouples ride‐status updates from notification logic. New observers can subscribe without touching core ride code.

    Decorator (FareCalculator + SurgePricingDecorator & DiscountDecorator):
        Allows dynamic composition of fare‐calculation behaviors (e.g., apply both surge and discount).

4. Extensibility & Future Features
    Scheduled Rides: 
        We can add a ScheduledRide subclass and modify RideFactory to return the appropriate subtype based on a timestamp in RideRequest.

    Driver Ratings/Reviews:
        We already store Driver.rating. After ride completion, we could prompt the Rider to rate the Driver. Those new methods can be added without changing the core matching logic—matching strategies can simply read updated ratings.

    Multiple Cities / Geospatial Indexing:
        Currently we use a linear scan to choose the nearest driver. For larger scale, we could integrate a spatial index (e.g., KD‐tree). The NearestDriverStrategy would be swapped with a new implementation that queries the index.

    Payment Integrations:
        We abstracted payment behind PaymentProcessor. We can add StripePaymentProcessor or WalletPaymentProcessor by implementing the interface.

    Promotions & Loyalty:
        We can add a new PromotionalFareDecorator to the fare calculation chain.

    Cancellation Flow:
        If Rider or Driver cancels mid‐ride, we can call ride.updateStatus(RideStatus.CANCELLED) and implement a cancellation fee or refund logic in completeRide or a specialized cancelRide(...) method in DispatchService.

5. Assumptions & Trade‐offs
    Distance Calculation:
        We use a simple Euclidean distance for demo. In reality, one would use a proper map API or Haversine formula.

    Driver Acceptance/Rejection:
        We assume drivers always accept. To support rejection, MatchingStrategy.chooseDriver(...) could simulate a “random reject” or we could introduce a callback driverOffer(ride) → boolean. If rejected, strategy would loop to the next best driver.

    Threading & Concurrency:
        Single‐threaded demo. In a multi‐threaded environment, we’d need proper synchronization for availableDrivers, update of driver status, and ride state changes.

    Carpooling:
        We did not implement pooling. To support carpooling, we could add a new CarpoolingStrategy that matches multiple riders to a single driver. The Ride class might become SharedRide with a list of Riders. This would require new data structures but would not change existing classes.

    Surge Activation:
        In this prototype, DispatchService.activateSurge(...) is manually called. In a real system, surge would be triggered by demand‐supply imbalance. This logic is not implemented (outside scope).