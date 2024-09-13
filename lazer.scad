//$fa=1;
$fs=.1;

rotate([0,-90,0])translate([3,0,0])union(){
	intersection(){
		cylinder(3,6/2,10/2);
		scale([1,10/6,1])cylinder(3,3,3);
		
	}
	cylinder(h=6,d=6);
	translate([0,0,5.5]){
		for(i=[0,180])rotate([0,0,i])translate([-3,0,0])rotate([-2,0,0]){
			translate([0,-1,0])cube([6,11,2]);
			translate([0,10,-1])cube([6,1,3]);
		}
		difference(){
			translate([-3,-1.5,0])cube([4,5,10]);
			translate([-1,0,6])sphere(d=5.5);
		}
	}
}

translate([20,0,3])rotate([0,90,0]){
	sphere(d=6);
	cylinder(d=3,h=8);
	translate([0,0,10])difference(){
		cube([6,8,6],center=true);
		rotate([0,90,0])translate([-1,0,0])cylinder(d=6,h=10,center=true);
	}
}