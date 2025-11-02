<?php

// Test script for zdump extension

echo "=== Testing zdump() ===\n\n";

// Test 1: Simple types
echo "Test 1: Simple types\n";
zdump(null);
zdump(true);
zdump(false);
zdump(42);
zdump(3.14159);
zdump("Hello, World!");

// Test 2: Arrays
echo "\nTest 2: Arrays\n";
zdump([1, 2, 3]);
zdump(['name' => 'John', 'age' => 30, 'city' => 'New York']);
zdump([
    'user' => [
        'name' => 'Alice',
        'email' => 'alice@example.com',
        'roles' => ['admin', 'user']
    ],
    'active' => true,
    'score' => 95.5
]);

// Test 3: Objects
echo "\nTest 3: Objects\n";

class Person {
    public $name;
    protected $age;
    private $ssn;
    
    public function __construct($name, $age, $ssn) {
        $this->name = $name;
        $this->age = $age;
        $this->ssn = $ssn;
    }
}

$person = new Person('Bob', 25, '123-45-6789');
zdump($person);

// Test 4: Nested structures
echo "\nTest 4: Nested structures\n";
$complex = [
    'level1' => [
        'level2' => [
            'level3' => [
                'deep' => 'value'
            ]
        ]
    ],
    'objects' => [
        new Person('Charlie', 35, '987-65-4321'),
        new Person('Diana', 28, '555-55-5555')
    ],
    'mixed' => [
        42,
        'string',
        true,
        null,
        [1, 2, 3]
    ]
];
zdump($complex);

// Test 5: Large array (test max_children limit)
echo "\nTest 5: Large array\n";
$large = range(1, 200);
zdump($large);

echo "\nAll tests completed!\n";
