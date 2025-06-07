#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <algorithm>

using namespace std;

// Enums
enum VehicleType { BIKE, SEDAN, SUV, AUTO };
enum RideStatus {
    REQUESTED,
    DRIVER_ASSIGNED,
    EN_ROUTE_TO_PICKUP,
    IN_PROGRESS,
    COMPLETED,
    CANCELLED
};
enum DriverStatus { AVAILABLE, ON_TRIP, OFFLINE };

// Declaring the classes
class Ride;
class Rider;
class Driver;

// Location
class Location {
public:
    double latitude;
    double longitude;

    Location(double lat = 0.0, double lon = 0.0)
        : latitude(lat), longitude(lon) {}

    double distanceTo(const Location& other) const {
        double dx = latitude - other.latitude;
        double dy = longitude - other.longitude;
        return std::sqrt(dx * dx + dy * dy);
    }
};

// Abstract User
class User {
protected:
    static int idCounter;
    int id;
    string name;
    string phone;

public:
    User(const string& name_, const string& phone_)
        : name(name_), phone(phone_) {
        id = ++idCounter;
    }

    int getId() const { return id; }
    string getName() const { return name; }
    string getPhone() const { return phone; }
};

int User::idCounter = 0;

// Abstract RideObserver
class RideObserver {
public:
    virtual void onRideStatusChanged(Ride* ride, RideStatus newStatus) = 0;
};

class DispatchService;

// Rider
class Rider : public User {
    Location currentLocation;
    vector<Ride*> rideHistory;
    double discountAmount = 0.0;

public:
    Rider(const string& name_, const string& phone_, const Location& loc)
        : User(name_, phone_), currentLocation(loc) {}

    Location getCurrentLocation() const { return currentLocation; }
    void updateLocation(const Location& loc) { currentLocation = loc; }

    void addRideToHistory(Ride* ride) {
        rideHistory.push_back(ride);
    }

    bool hasDiscount() const { return discountAmount > 0.0; }
    double getDiscountAmount() const { return discountAmount; }
    void setDiscountAmount(double amt) { discountAmount = amt; }

    Ride* requestRide(const Location& pickup, const Location& drop, VehicleType type);
};

// Vehicle
class Vehicle {
    string plateNumber;
    VehicleType type;
    int capacity;
    double farePerKm;

public:
    Vehicle(const string& plate, VehicleType type_, int cap, double rate)
        : plateNumber(plate), type(type_), capacity(cap), farePerKm(rate) {}

    VehicleType getType() const { return type; }
    double getFarePerKm() const { return farePerKm; }
    int getCapacity() const { return capacity; }

    string getPlateNumber() const { return plateNumber; }
};

// Driver
class Driver : public User {
    Vehicle* vehicle;
    Location currentLocation;
    DriverStatus status;
    double rating;

public:
    Driver(const string& name_, const string& phone_,
           Vehicle* vehicle_, const Location& loc, double rating_)
        : User(name_, phone_), vehicle(vehicle_), currentLocation(loc),
          status(AVAILABLE), rating(rating_) {}

    Location getCurrentLocation() const { return currentLocation; }
    void updateLocation(const Location& loc) { currentLocation = loc; }

    DriverStatus getStatus() const { return status; }
    void setStatus(DriverStatus s) { status = s; }

    Vehicle* getVehicle() const { return vehicle; }
    double getRating() const { return rating; }
    void setRating(double r) { rating = r; }

    friend ostream& operator<<(ostream& os, const Driver& d) {
        os << "Driver{name='" << d.name
           << "', vehicle=";
        switch (d.vehicle->getType()) {
            case BIKE:  os << "BIKE";  break;
            case SEDAN: os << "SEDAN"; break;
            case SUV:   os << "SUV";   break;
            case AUTO:  os << "AUTO";  break;
        }
        os << ", loc=(" << d.currentLocation.latitude << ", "
           << d.currentLocation.longitude << "), rating="
           << d.rating << "}";
        return os;
    }
};

// RideRequest
class RideRequest {
    Rider* rider;
    Location pickup;
    Location drop;
    VehicleType type;

public:
    RideRequest(Rider* r, const Location& p,
                const Location& d, VehicleType t)
        : rider(r), pickup(p), drop(d), type(t) {}

    Rider* getRider() const { return rider; }
    Location getPickup() const { return pickup; }
    Location getDrop() const { return drop; }
    VehicleType getType() const { return type; }
};

// Implementing Ride Observer
class RiderNotificationService : public RideObserver {
public:
    void onRideStatusChanged(Ride* ride, RideStatus newStatus) override;
};

class DriverNotificationService : public RideObserver {
public:
    void onRideStatusChanged(Ride* ride, RideStatus newStatus) override;
};

// FareCalculator
class FareCalculator {
public:
    virtual double calculate(Ride* ride) const = 0;
};

class BaseFareCalculator : public FareCalculator {
    static const double BASE_FARE = 50.0;

public:
    double calculate(Ride* ride) const override;
};

class FareDecorator : public FareCalculator {
protected:
    FareCalculator* wrappedCalculator;

public:
    FareDecorator(FareCalculator* calc)
        : wrappedCalculator(calc) {}
};

class SurgePricingDecorator : public FareDecorator {
    double surgeMultiplier;

public:
    SurgePricingDecorator(FareCalculator* calc, double mult)
        : FareDecorator(calc), surgeMultiplier(mult) {}

    double calculate(Ride* ride) const override;
};

class DiscountDecorator : public FareDecorator {
    double discountAmount;

public:
    DiscountDecorator(FareCalculator* calc, double discount)
        : FareDecorator(calc), discountAmount(discount) {}

    double calculate(Ride* ride) const override;
};

// Abstract PaymentProcessor
class PaymentProcessor {
public:
    virtual bool processPayment(Ride* ride, double amount) = 0;
};

class DummyPaymentProcessor : public PaymentProcessor {
public:
    bool processPayment(Ride* ride, double amount) override;
};

// MatchingStrategy
class MatchingStrategy {
public:
    virtual Driver* chooseDriver(const RideRequest& request, const vector<Driver*>& availableDrivers) = 0;
};

class NearestDriverStrategy : public MatchingStrategy {
public:
    Driver* chooseDriver(const RideRequest& request, const vector<Driver*>& availableDrivers) override;
};

class BestRatedDriverStrategy : public MatchingStrategy {
public:
    Driver* chooseDriver(const RideRequest& request, const vector<Driver*>& availableDrivers) override;
};

// RideFactory
class RideFactory {
    static int rideCounter;

public:
    static Ride* createRide(const RideRequest& request);
};

int RideFactory::rideCounter = 0;

// Ride
class Ride {
    string id;
    Rider* rider;
    Driver* driver;
    Location pickupLocation;
    Location dropLocation;
    VehicleType requestedType;
    RideStatus status;
    double distanceKm;
    double fare;
    bool paid;

    vector<RideObserver*> observers;

public:
    Ride(const string& rideId, Rider* r,
         const Location& pickup, const Location& drop, VehicleType type)
        : id(rideId),
          rider(r),
          driver(nullptr),
          pickupLocation(pickup),
          dropLocation(drop),
          requestedType(type),
          status(REQUESTED),
          distanceKm(pickup.distanceTo(drop)),
          fare(0.0),
          paid(false) {}

    string getId() const { return id; }
    Rider* getRider() const { return rider; }
    Driver* getDriver() const { return driver; }
    Location getPickupLocation() const { return pickupLocation; }
    Location getDropLocation() const { return dropLocation; }
    RideStatus getStatus() const { return status; }
    double getDistanceKm() const { return distanceKm; }
    double getFare() const { return fare; }
    bool isPaid() const { return paid; }

    void assignDriver(Driver* d);
    void attachObserver(RideObserver* obs);
    void removeObserver(RideObserver* obs);
    void notifyObservers(RideStatus newStatus);
    void updateStatus(RideStatus newStatus);
    void setFare(double f) { fare = f; }
    void setPaid(bool p) { paid = p; }
};

// DispatchService
class DispatchService {
    vector<Driver*> availableDrivers;
    map<string, Ride*> ongoingRides;
    vector<Ride*> completedRides;

    MatchingStrategy* matchingStrategy;
    PaymentProcessor* paymentProcessor;

    bool surgeActive;
    double surgeMultiplier;

    DispatchService()
        : matchingStrategy(new NearestDriverStrategy()), paymentProcessor(new DummyPaymentProcessor()), surgeActive(false), surgeMultiplier(1.0) {}

public:
    // Delete copy/move constructors
    DispatchService(const DispatchService&) = delete;
    DispatchService& operator=(const DispatchService&) = delete;

    static DispatchService& getInstance() {
        static DispatchService instance;
        return instance;
    }

    void setMatchingStrategy(MatchingStrategy* strategy) {
        if (matchingStrategy) delete matchingStrategy;
        matchingStrategy = strategy;
    }

    void activateSurge(double multiplier) {
        surgeActive = true;
        surgeMultiplier = multiplier;
    }

    void deactivateSurge() {
        surgeActive = false;
        surgeMultiplier = 1.0;
    }

    bool isSurge() const { return surgeActive; }
    double getCurrentMultiplier() const { return surgeMultiplier; }

    void registerDriver(Driver* driver) {
        driver->setStatus(AVAILABLE);
        availableDrivers.push_back(driver);
        cout << "Driver registered: " << *driver << endl;
    }

    void deregisterDriver(Driver* driver) {
        driver->setStatus(OFFLINE);
        availableDrivers.erase(
            remove(availableDrivers.begin(), availableDrivers.end(), driver),
            availableDrivers.end());
        cout << "Driver deregistered: " << *driver << endl;
    }

    Ride* requestRide(Rider* rider, const Location& pickup, const Location& drop, VehicleType type) {
        cout << "\n=== Rider " << rider->getName() << " requests a "
             << (type == BIKE   ? "BIKE" :
                 type == SEDAN  ? "SEDAN" :
                 type == SUV    ? "SUV"   : "AUTO")
             << " ride ===" << endl;

        RideRequest request(rider, pickup, drop, type);
        Ride* ride = RideFactory::createRide(request);
        rider->addRideToHistory(ride);

        // Choose a driver
        Driver* chosenDriver =
            matchingStrategy->chooseDriver(request, availableDrivers);
        if (!chosenDriver) {
            cout << "No available drivers for Ride " << ride->getId()
                 << ". Cancelling ride." << endl;
            ride->updateStatus(CANCELLED);
            return ride;
        }

        // Assign driver
        ride->attachObserver(new RiderNotificationService());
        ride->attachObserver(new DriverNotificationService());
        ride->assignDriver(chosenDriver);

        // Update driver status & remove from available pool
        chosenDriver->setStatus(ON_TRIP);
        availableDrivers.erase(
            remove(availableDrivers.begin(), availableDrivers.end(), chosenDriver),
            availableDrivers.end());

        // Save in ongoing rides
        ongoingRides[ride->getId()] = ride;
        return ride;
    }

    void updateRideStatus(const string& rideId, RideStatus newStatus) {
        auto it = ongoingRides.find(rideId);
        if (it == ongoingRides.end()) {
            cout << "Ride " << rideId
                 << " not found or already completed." << endl;
            return;
        }
        it->second->updateStatus(newStatus);
    }

    void completeRide(const string& rideId) {
        auto it = ongoingRides.find(rideId);
        if (it == ongoingRides.end()) {
            cout << "Ride " << rideId
                 << " not found or already completed." << endl;
            return;
        }
        Ride* ride = it->second;

        // 1. Mark completed
        ride->updateStatus(COMPLETED);

        // 2. Fare Calculation
        FareCalculator* calculator = new BaseFareCalculator();
        if (surgeActive) {
            calculator = new SurgePricingDecorator(calculator, surgeMultiplier);
        }
        if (ride->getRider()->hasDiscount()) {
            calculator = new DiscountDecorator(
                calculator, ride->getRider()->getDiscountAmount());
        }
        double finalFare = calculator->calculate(ride);
        delete calculator;
        ride->setFare(finalFare);

        // 3. Process payment
        bool paid = paymentProcessor->processPayment(ride, finalFare);
        if (paid) {
            ride->setPaid(true);
            cout << "[Notification to Rider " << ride->getRider()->getName()
                 << "]: Payment of ₹" << finalFare
                 << " successful." << endl;
        } else {
            cout << "Payment failed for Ride " << ride->getId() << endl;
        }

        // 4. Free up driver
        Driver* driver = ride->getDriver();
        driver->setStatus(AVAILABLE);
        availableDrivers.push_back(driver);
        cout << "Driver " << driver->getName() << " is now AVAILABLE." << endl;

        // 5. Move ride from ongoing to completed
        ongoingRides.erase(it);
        completedRides.push_back(ride);
        cout << "Ride " << rideId << " completed and archived.\n" << endl;
    }

    void printAvailableDrivers() const {
        cout << "\n--- Available Drivers ---" << endl;
        for (const auto& d : availableDrivers) {
            cout << *d << endl;
        }
        cout << "-------------------------" << endl;
    }
};

// Implementation Details
Ride* Rider::requestRide(const Location& pickup, const Location& drop, VehicleType type) {
    return DispatchService::getInstance().requestRide(this, pickup, drop, type);
}
void RiderNotificationService::onRideStatusChanged(Ride* ride, RideStatus newStatus) {
    cout << "[Notification to Rider " << ride->getRider()->getName()
         << "]: Ride " << ride->getId() << " is now ";
    switch (newStatus) {
        case REQUESTED:           cout << "REQUESTED";           break;
        case DRIVER_ASSIGNED:     cout << "DRIVER_ASSIGNED";     break;
        case EN_ROUTE_TO_PICKUP:  cout << "EN_ROUTE_TO_PICKUP";  break;
        case IN_PROGRESS:         cout << "IN_PROGRESS";         break;
        case COMPLETED:           cout << "COMPLETED";           break;
        case CANCELLED:           cout << "CANCELLED";           break;
    }
    cout << endl;
}

void DriverNotificationService::onRideStatusChanged(Ride* ride, RideStatus newStatus) {
    if (ride->getDriver()) {
        cout << "[Notification to Driver " << ride->getDriver()->getName()
             << "]: Ride " << ride->getId() << " is now ";
        switch (newStatus) {
            case REQUESTED:           cout << "REQUESTED";           break;
            case DRIVER_ASSIGNED:     cout << "DRIVER_ASSIGNED";     break;
            case EN_ROUTE_TO_PICKUP:  cout << "EN_ROUTE_TO_PICKUP";  break;
            case IN_PROGRESS:         cout << "IN_PROGRESS";         break;
            case COMPLETED:           cout << "COMPLETED";           break;
            case CANCELLED:           cout << "CANCELLED";           break;
        }
        cout << endl;
    }
}

double BaseFareCalculator::calculate(Ride* ride) const {
    double distance = ride->getDistanceKm();
    double perKmRate = ride->getDriver()->getVehicle()->getFarePerKm();
    return BASE_FARE + (distance * perKmRate);
}

double SurgePricingDecorator::calculate(Ride* ride) const {
    double baseFare = wrappedCalculator->calculate(ride);
    return baseFare * surgeMultiplier;
}

double DiscountDecorator::calculate(Ride* ride) const {
    double fareBeforeDiscount = wrappedCalculator->calculate(ride);
    double finalFare = fareBeforeDiscount - discountAmount;
    return (finalFare < 0.0 ? 0.0 : finalFare);
}

bool DummyPaymentProcessor::processPayment(Ride* ride, double amount) {
    cout << "Processing payment of ₹" << amount
         << " for Ride " << ride->getId() << endl;
    return true;
}

// Matching Strategies
Driver* NearestDriverStrategy::chooseDriver(
    const RideRequest& request,
    const vector<Driver*>& availableDrivers) {

    Driver* nearest = nullptr;
    double minDistance = numeric_limits<double>::infinity();
    for (auto d : availableDrivers) {
        if (d->getStatus() == AVAILABLE &&
            d->getVehicle()->getType() == request.getType()) {
            double dist = d->getCurrentLocation().distanceTo(request.getPickup());
            if (dist < minDistance) {
                minDistance = dist;
                nearest = d;
            }
        }
    }
    return nearest;
}

Driver* BestRatedDriverStrategy::chooseDriver(
    const RideRequest& request,
    const vector<Driver*>& availableDrivers) {

    Driver* best = nullptr;
    double bestRating = -1.0;
    for (auto d : availableDrivers) {
        if (d->getStatus() == AVAILABLE &&
            d->getVehicle()->getType() == request.getType()) {
            if (d->getRating() > bestRating) {
                bestRating = d->getRating();
                best = d;
            }
        }
    }
    return best;
}

// RideFactory
Ride* RideFactory::createRide(const RideRequest& request) {
    string rideId = to_string(++rideCounter);
    return new Ride(rideId,
                    request.getRider(),
                    request.getPickup(),
                    request.getDrop(),
                    request.getType());
}

// Ride methods
void Ride::assignDriver(Driver* d) {
    driver = d;
    updateStatus(DRIVER_ASSIGNED);
}

void Ride::attachObserver(RideObserver* obs) {
    observers.push_back(obs);
}

void Ride::removeObserver(RideObserver* obs) {
    observers.erase(
        remove(observers.begin(), observers.end(), obs),
        observers.end());
}

void Ride::notifyObservers(RideStatus newStatus) {
    for (auto obs : observers) {
        obs->onRideStatusChanged(this, newStatus);
    }
}

void Ride::updateStatus(RideStatus newStatus) {
    status = newStatus;
    notifyObservers(newStatus);
}

// Main Function
int main() {
    DispatchService& dispatch = DispatchService::getInstance();

    // Create some vehicles and drivers, then register them
    Vehicle* v1 = new Vehicle("KA-01-1234", SEDAN, 4, 15.0);
    Vehicle* v2 = new Vehicle("KA-01-5678", SEDAN, 4, 15.0);
    Vehicle* v3 = new Vehicle("KA-02-1122", SUV,   6, 20.0);
    Vehicle* v4 = new Vehicle("KA-02-3344", AUTO,  3, 10.0);

    Driver* d1 = new Driver("Alice",   "9999990001", v1, Location(12.9716, 77.5946), 4.8);
    Driver* d2 = new Driver("Bob",     "9999990002", v2, Location(12.9750, 77.5900), 4.9);
    Driver* d3 = new Driver("Charlie", "9999990003", v3, Location(12.9700, 77.6000), 4.7);
    Driver* d4 = new Driver("Dave",    "9999990004", v4, Location(12.9720, 77.5950), 4.5);

    dispatch.registerDriver(d1);
    dispatch.registerDriver(d2);
    dispatch.registerDriver(d3);
    dispatch.registerDriver(d4);

    dispatch.printAvailableDrivers();

    // Create a rider
    Rider* rider1 = new Rider("Eve", "8888880001", Location(12.9725, 77.5930));

    // Rider requests a Sedan ride
    Ride* ride1 = rider1->requestRide(
        Location(12.9725, 77.5930),  // pickup
        Location(12.9850, 77.5950),  // drop
        SEDAN);

    // Progress through ride statuses
    dispatch.updateRideStatus(ride1->getId(), EN_ROUTE_TO_PICKUP);
    dispatch.updateRideStatus(ride1->getId(), IN_PROGRESS);

    // Activate surge pricing
    dispatch.activateSurge(1.5);
    cout << "\n--- Surge pricing activated (1.5x) ---\n" << endl;

    // Complete the ride
    dispatch.completeRide(ride1->getId());

    dispatch.printAvailableDrivers();

    // Switch matching strategy at runtime
    cout << "\n--- Switching to BestRatedDriverStrategy ---\n" << endl;
    dispatch.setMatchingStrategy(new BestRatedDriverStrategy());

    // Create another rider & request SUV ride
    Rider* rider2 = new Rider("Frank", "8888880002", Location(12.9740, 77.5960));
    Ride* ride2 = rider2->requestRide(
        Location(12.9740, 77.5960),
        Location(12.9800, 77.6000),
        SUV);

    dispatch.updateRideStatus(ride2->getId(), EN_ROUTE_TO_PICKUP);
    dispatch.updateRideStatus(ride2->getId(), IN_PROGRESS);
    dispatch.completeRide(ride2->getId());

    dispatch.printAvailableDrivers();

    delete rider1;
    delete rider2;

    return 0;
}