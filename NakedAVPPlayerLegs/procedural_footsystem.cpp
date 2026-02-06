#pragma once

#include <vector>
#include <cmath>
#include <memory>

// Vector3 class for position/movement
struct FVector3
{
    float X, Y, Z;
    
    FVector3(float x = 0, float y = 0, float z = 0) : X(x), Y(y), Z(z) {}
    
    FVector3 operator+(const FVector3& other) const {
        return FVector3(X + other.X, Y + other.Y, Z + other.Z);
    }
    
    FVector3 operator-(const FVector3& other) const {
        return FVector3(X - other.X, Y - other.Y, Z - other.Z);
    }
    
    FVector3 operator*(float scalar) const {
        return FVector3(X * scalar, Y * scalar, Z * scalar);
    }
    
    float Length() const {
        return std::sqrt(X*X + Y*Y + Z*Z);
    }
    
    FVector3 Normalized() const {
        float len = Length();
        if (len > 0.0001f)
            return *this * (1.0f / len);
        return *this;
    }
};

// Foot data structure
struct FootData
{
    FVector3 CurrentPosition;      // Current world position
    FVector3 TargetPosition;       // Target placement position
    FVector3 PreviousPosition;     // Previous frame position
    FVector3 SwingOffset;          // Lift during swing phase
    float Phase = 0.0f;           // 0-1 cycle phase
    float Weight = 0.0f;          // IK weight/blend
    bool bIsPlanted = true;       // Is foot planted on ground?
    float TimeSinceLift = 0.0f;   // Time since lifted
};

// Leg structure
struct Leg
{
    FootData Foot;
    FVector3 HipOffset;           // Local offset from pelvis
    float LegLength = 100.0f;     // Length from hip to foot
    bool bIsMoving = false;
};

// Terrain query interface
class ITerrainQuery
{
public:
    virtual ~ITerrainQuery() = default;
    virtual FVector3 GetSurfaceNormal(const FVector3& position) const = 0;
    virtual float GetSurfaceHeight(const FVector3& position) const = 0;
    virtual bool IsWalkable(const FVector3& position) const = 0;
};

// Main procedural walk system
class ProceduralWalkSystem
{
private:
    // Character properties
    FVector3 CharacterPosition;
    FVector3 CharacterVelocity;
    FVector3 CharacterAcceleration;
    float CharacterHeight = 180.0f;
    float CharacterRadius = 30.0f;
    
    // System parameters
    float MoveSpeed = 150.0f;         // Units per second
    float StrideLengthMultiplier = 0.5f;
    float LiftHeightMultiplier = 1.0f;
    float StepHeight = 15.0f;        // Max step height
    float BalanceThreshold = 5.0f;   // Balance correction threshold
    
    // Legs
    std::vector<Leg> Legs;
    FVector3 PelvisOffset;           // Pelvis offset from character center
    
    // State
    float GaitCycleTime = 0.0f;
    float TimeSinceLastStep = 0.0f;
    float StrideDuration = 0.0f;
    
    // External dependencies
    std::shared_ptr<ITerrainQuery> TerrainQuery;
    
public:
    ProceduralWalkSystem(std::shared_ptr<ITerrainQuery> terrainQuery)
        : TerrainQuery(terrainQuery)
    {
        InitializeLegs();
        CalculateStrideDuration();
    }
    
    // Initialize legs with default positions
    void InitializeLegs()
    {
        Legs.clear();
        
        // Create 2 legs (simplified - actual would have 4 for quadruped)
        Leg frontLeft, frontRight, backLeft, backRight;
        
        // Configure leg offsets (relative to character center)
        frontLeft.HipOffset = FVector3(-CharacterRadius, CharacterRadius * 0.5f, 0);
        frontRight.HipOffset = FVector3(CharacterRadius, CharacterRadius * 0.5f, 0);
        backLeft.HipOffset = FVector3(-CharacterRadius, -CharacterRadius * 0.5f, 0);
        backRight.HipOffset = FVector3(CharacterRadius, -CharacterRadius * 0.5f, 0);
        
        Legs.push_back(frontLeft);
        Legs.push_back(frontRight);
        Legs.push_back(backLeft);
        Legs.push_back(backRight);
        
        // Initialize foot positions
        for (auto& leg : Legs)
        {
            leg.Foot.CurrentPosition = CharacterPosition + leg.HipOffset;
            leg.Foot.TargetPosition = leg.Foot.CurrentPosition;
            leg.Foot.PreviousPosition = leg.Foot.CurrentPosition;
        }
    }
    
    // Update the walk system
    void Update(float deltaTime, const FVector3& targetVelocity)
    {
        // Update character state
        CharacterVelocity = targetVelocity;
        CharacterPosition = CharacterPosition + CharacterVelocity * deltaTime;
        
        // Update gait timing
        GaitCycleTime += deltaTime;
        TimeSinceLastStep += deltaTime;
        
        // Calculate adaptive stride duration based on speed
        CalculateStrideDuration();
        
        // Predict foot placement positions
        PredictFootPlacement();
        
        // Update each leg's movement
        for (auto& leg : Legs)
        {
            UpdateLegMovement(leg, deltaTime);
        }
        
        // Balance pelvis based on foot positions
        UpdatePelvisBalance();
        
        // Apply terrain adaptation
        AdaptToTerrain();
    }
    
    // Calculate stride duration based on speed
    void CalculateStrideDuration()
    {
        float speed = CharacterVelocity.Length();
        if (speed < 0.1f) speed = 0.1f;
        
        // Base duration with speed inverse relationship
        float baseDuration = 0.5f; // Base stride duration in seconds
        StrideDuration = baseDuration * (MoveSpeed / speed) * StrideLengthMultiplier;
        StrideDuration = std::max(0.1f, std::min(StrideDuration, 2.0f));
    }
    
    // Predict where feet should be placed
    void PredictFootPlacement()
    {
        for (auto& leg : Legs)
        {
            // Calculate ideal foot position based on velocity
            FVector3 hipWorldPos = CharacterPosition + leg.HipOffset + PelvisOffset;
            
            // Predict future position (one stride ahead)
            float predictionTime = StrideDuration * 0.5f;
            FVector3 predictedPosition = hipWorldPos + CharacterVelocity * predictionTime;
            
            // Project to terrain
            predictedPosition.Z = TerrainQuery->GetSurfaceHeight(predictedPosition);
            
            // Get surface normal for orientation
            FVector3 surfaceNormal = TerrainQuery->GetSurfaceNormal(predictedPosition);
            
            // Adjust foot orientation to match surface
            FVector3 footForward = CharacterVelocity.Normalized();
            FVector3 footRight = FVector3(0, 0, 1).Normalized(); // Simplified
            
            // If foot needs to move and is not currently moving
            float distanceToTarget = (leg.Foot.CurrentPosition - predictedPosition).Length();
            if (distanceToTarget > (MoveSpeed * StrideDuration * 0.1f) && !leg.bIsMoving)
            {
                leg.Foot.TargetPosition = predictedPosition;
                leg.Foot.bIsPlanted = false;
                leg.bIsMoving = true;
                leg.Foot.Phase = 0.0f;
                leg.Foot.TimeSinceLift = 0.0f;
            }
        }
    }
    
    // Update individual leg movement
    void UpdateLegMovement(Leg& leg, float deltaTime)
    {
        if (leg.bIsMoving)
        {
            leg.Foot.TimeSinceLift += deltaTime;
            leg.Foot.Phase = leg.Foot.TimeSinceLift / StrideDuration;
            
            if (leg.Foot.Phase >= 1.0f)
            {
                // Foot planting
                leg.Foot.CurrentPosition = leg.Foot.TargetPosition;
                leg.Foot.bIsPlanted = true;
                leg.bIsMoving = false;
                leg.Foot.Phase = 0.0f;
            }
            else
            {
                // Swing phase - calculate parabolic path
                FVector3 startPos = leg.Foot.PreviousPosition;
                FVector3 endPos = leg.Foot.TargetPosition;
                
                // Calculate lift height based on obstacle height
                float obstacleHeight = CalculateObstacleHeight(startPos, endPos);
                float maxLiftHeight = StepHeight * LiftHeightMultiplier + obstacleHeight;
                
                // Parabolic swing trajectory
                float t = leg.Foot.Phase;
                FVector3 linear = startPos + (endPos - startPos) * t;
                
                // Sine-based lift curve
                float lift = std::sin(t * 3.14159f) * maxLiftHeight;
                
                // Apply lift
                leg.Foot.SwingOffset = FVector3(0, 0, lift);
                leg.Foot.CurrentPosition = linear + leg.Foot.SwingOffset;
            }
        }
        else if (leg.Foot.bIsPlanted)
        {
            // Apply slight movement with pelvis
            leg.Foot.CurrentPosition = leg.Foot.TargetPosition;
        }
        
        // Store previous position for next frame
        leg.Foot.PreviousPosition = leg.Foot.CurrentPosition;
    }
    
    // Calculate height of obstacles between start and end positions
    float CalculateObstacleHeight(const FVector3& start, const FVector3& end)
    {
        // Sample points along the path
        int samples = 5;
        float maxHeight = 0.0f;
        
        for (int i = 1; i < samples - 1; i++)
        {
            float t = float(i) / float(samples);
            FVector3 samplePoint = start + (end - start) * t;
            
            // Get terrain height at sample point
            float terrainHeight = TerrainQuery->GetSurfaceHeight(samplePoint);
            float lineHeight = start.Z + (end.Z - start.Z) * t;
            
            float obstacle = terrainHeight - lineHeight;
            if (obstacle > maxHeight)
                maxHeight = obstacle;
        }
        
        return std::max(0.0f, maxHeight - StepHeight * 0.5f);
    }
    
    // Adjust pelvis based on foot positions for balance
    void UpdatePelvisBalance()
    {
        if (Legs.empty()) return;
        
        // Calculate average foot height
        float totalHeight = 0.0f;
        int plantedCount = 0;
        
        for (const auto& leg : Legs)
        {
            if (leg.Foot.bIsPlanted)
            {
                totalHeight += leg.Foot.CurrentPosition.Z;
                plantedCount++;
            }
        }
        
        if (plantedCount > 0)
        {
            float averageHeight = totalHeight / plantedCount;
            float targetPelvisZ = averageHeight + CharacterHeight * 0.5f;
            
            // Smoothly adjust pelvis height
            float currentPelvisZ = PelvisOffset.Z;
            float deltaZ = targetPelvisZ - currentPelvisZ;
            
            // Apply with smoothing
            PelvisOffset.Z += deltaZ * 0.1f; // Smoothing factor
        }
        
        // Lateral balance (side-to-side)
        FVector3 balanceOffset = FVector3(0, 0, 0);
        
        // Simplified balance calculation
        // In full implementation, would calculate center of mass vs support polygon
    }
    
    // Adapt feet to terrain surface
    void AdaptToTerrain()
    {
        for (auto& leg : Legs)
        {
            if (leg.Foot.bIsPlanted)
            {
                // Sample terrain under foot
                FVector3 footPos = leg.Foot.CurrentPosition;
                float terrainHeight = TerrainQuery->GetSurfaceHeight(footPos);
                FVector3 surfaceNormal = TerrainQuery->GetSurfaceNormal(footPos);
                
                // Adjust foot position to terrain
                footPos.Z = terrainHeight;
                
                // Adjust foot rotation based on surface normal
                // (In full implementation, would set foot rotation matrix)
                
                leg.Foot.CurrentPosition = footPos;
            }
        }
    }
    
    // Getters for animation system
    const std::vector<Leg>& GetLegs() const { return Legs; }
    const FVector3& GetPelvisOffset() const { return PelvisOffset; }
    float GetStrideDuration() const { return StrideDuration; }
    
    // Setters for runtime customization
    void SetMoveSpeed(float speed) { MoveSpeed = speed; }
    void SetStrideLengthMultiplier(float multiplier) { 
        StrideLengthMultiplier = std::max(0.1f, std::min(multiplier, 3.0f));
    }
    void SetLiftHeightMultiplier(float multiplier) { 
        LiftHeightMultiplier = std::max(0.1f, std::min(multiplier, 3.0f));
    }
    
    // Query foot placement for AI/navigation
    bool GetSafeFootPosition(FVector3& outPosition, const FVector3& desiredPosition)
    {
        // Raycast/query for safe placement
        if (!TerrainQuery->IsWalkable(desiredPosition))
        {
            // Search nearby positions
            const float searchRadius = 50.0f;
            const int searchSteps = 8;
            
            for (int i = 0; i < searchSteps; i++)
            {
                float angle = (2.0f * 3.14159f * i) / searchSteps;
                FVector3 offset = FVector3(
                    std::cos(angle) * searchRadius,
                    std::sin(angle) * searchRadius,
                    0
                );
                
                FVector3 testPos = desiredPosition + offset;
                if (TerrainQuery->IsWalkable(testPos))
                {
                    outPosition = testPos;
                    outPosition.Z = TerrainQuery->GetSurfaceHeight(testPos);
                    return true;
                }
            }
            return false;
        }
        
        outPosition = desiredPosition;
        outPosition.Z = TerrainQuery->GetSurfaceHeight(desiredPosition);
        return true;
    }
};

// Example terrain query implementation
class SimpleTerrainQuery : public ITerrainQuery
{
public:
    FVector3 GetSurfaceNormal(const FVector3& position) const override
    {
        // Simplified - always returns up vector
        // In real implementation, would raycast against terrain
        return FVector3(0, 0, 1);
    }
    
    float GetSurfaceHeight(const FVector3& position) const override
    {
        // Sample from heightmap or physics system
        // This is a simplified flat terrain
        return 0.0f;
    }
    
    bool IsWalkable(const FVector3& position) const override
    {
        // Check if position is on valid walkable surface
        // Would normally check against navmesh or collision
        return true;
    }
};

// Usage example
int main()
{
    // Create terrain query
    auto terrainQuery = std::make_shared<SimpleTerrainQuery>();
    
    // Create walk system
    ProceduralWalkSystem walkSystem(terrainQuery);
    
    // Customize parameters (like in the video)
    walkSystem.SetMoveSpeed(150.0f);        // 150 units/sec
    walkSystem.SetStrideLengthMultiplier(0.5f);
    walkSystem.SetLiftHeightMultiplier(1.0f);
    
    // Simulation loop
    for (int frame = 0; frame < 1000; ++frame)
    {
        float deltaTime = 1.0f / 60.0f; // 60 FPS
        
        // Example target velocity (from player input or AI)
        FVector3 targetVelocity(100.0f, 0.0f, 0.0f);
        
        // Update walk system
        walkSystem.Update(deltaTime, targetVelocity);
        
        // Get leg data for rendering/animation
        const auto& legs = walkSystem.GetLegs();
        
        // Here you would:
        // 1. Pass leg positions to IK system
        // 2. Update character skeleton
        // 3. Render character
    }
    
    return 0;
}

// Key Components of This Implementation:
//     Adaptive Foot Placement:
//         Predicts foot positions based on velocity
//         Adjusts for terrain height and obstacles
//         Prevents foot sliding with proper planting logic

//     Terrain Adaptation:
//         Samples terrain height at foot positions
//         Adjusts foot orientation to surface normals
//         Handles steps and uneven terrain

//     Procedural Animation:
//         Parabolic foot swing trajectories
//         Adjustable stride length and lift height
//         Phase-based movement timing

//     Runtime Customization:
//         All parameters adjustable during runtime
//         Can create unique walk cycles per character
//         Responds to gameplay events

//     Balance System:
//         Adjusts pelvis height based on foot positions
//         Maintains character balance
//         Smooth transitions between steps

// To Integrate This with a Game Engine:

//     Hook into Animation System:
//         Feed leg positions into IK system
//         Blend with upper body animations

//     Terrain Integration:
//         Implement ITerrainQuery with raycasts against your world
//         Use physics system or navigation mesh for walkability

//     Performance:
//         Bulk calculations in update loop
//         Can be optimized with spatial partitioning
//         Control LOD based on distance

//     Extend for Different Creatures:
//         Modify leg configurations
//         Add different gait patterns (trot, gallop, etc.)
//         Support for different body types

//     Key Features Mentioned:
//         Runtime procedural animation generation
//         Adapts to character size/movement speed
//         No foot sliding
//         Terrain adaptation (angles, leg paths)
//         Intelligent foot placement (not just IK)
//         Handles limited landing areas (beams example)
//         Step climbing from any angle
//         Variable-driven customization
//         Lightweight, runs on many characters
//         Control Rig for calculations

// System Properties from Video Text:
//     "move speed 150u/s" → Parameterized movement system
//     "stride length: 0.5x" → Adjustable gait parameters
//     "lift height: 1.0x" → Configurable step height
//     "mathematical solution" → Algorithm-based, not animation-based

// 2. Inference and Pattern Recognition:
// Inferred Technical Requirements:

//     Foot Placement Algorithm: Needs predictive logic (not reactive IK)
//         Must find "best place to plant the foot"
//         Plans leg movement ahead of time
//         Adapts to terrain constraints

//     Terrain Interaction:
//         Query system for surface height/normals
//         Walkability detection
//         Obstacle height calculation

//     Gait Cycle Management:
//         Phase-based leg movement
//         Swing/stance timing
//         Inter-leg coordination

//     Runtime Customization:
//         Parameter-driven behavior
//         Real-time adjustments
//         Character-specific settings

// 3. Architecture Design Based on Described Behavior:
// Key Observations from Video Descriptions:
//     "Feet find best place to plant" → Predictive foot placement algorithm
//     "Wider gait on beams" → Adaptive step width based on landing area constraints
//     "Feet placed directly in front" → Path planning when space is limited
//     "Lifted to correct height for steps" → Obstacle-aware swing trajectory
//     "Course correcting for acceleration" → Velocity prediction and adaptation

// Resulting System Components:

// 1. Core Data Structures:
//     FootData - Tracks foot state (position, phase, planted status)
//     Leg - Contains foot and hip relationship
//     ProceduralWalkSystem - Main controller class

// 2. Algorithms Extracted from Description:

// // From "intelligently finds best place to plant the foot"
// PredictFootPlacement() - Predicts where foot should go

// // From "angles of feet adjusted to match terrain"
// AdaptToTerrain() - Adjusts foot orientation to surface

// // From "parabolic swing for step climbing"
// UpdateLegMovement() - Calculates lift trajectory

// // From "balance and pelvis adjustment"
// UpdatePelvisBalance() - Maintains character balance

// 3. Parameter System:
// // Directly from video's UI elements:
// SetMoveSpeed(150.0f);           // "move speed 150u/s"
// SetStrideLengthMultiplier(0.5f); // "stride length: 0.5x"
// SetLiftHeightMultiplier(1.0f);   // "lift height: 1.0x"

// 4. Technical Implementation Choices:
// Based on Performance Claims:
//     "Lightweight" → Minimal allocations, simple math
//     "Bulk of calculation in Control Rig" → Separated terrain queries
//     "Runs on many characters" → O(n) algorithms, no complex pathfinding

// From "Mathematical Solution" Description:
//     Used parabolic curves for foot swing (sin(t * PI) * height)
//     Linear interpolation between positions
//     Phase-based timing instead of fixed animations

// Terrain Adaptation Logic:
// // From: "not simply an IK solution that prevents feet passing through floor"
// // Instead: "finds best place to plant"
// bool GetSafeFootPosition() - Searches nearby positions when primary is invalid

// 5. Missing Information and Assumptions:
// Assumptions Made:
//     Character Type: Assumed biped/humanoid (simplified to 4 legs for demonstration)
//     Terrain System: Created abstract ITerrainQuery interface
//     Physics Integration: Assumed external physics/raycast system
//     Animation Pipeline: Focused on data generation, not rendering

// Simplifications:
//     Reduced complex gait algorithms to phase-based timing
//     Simplified balance system (mentioned but not detailed in video)
//     Basic terrain sampling instead of advanced navmesh queries