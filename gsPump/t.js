/**
 * Macro for data evaluation of RIDES1 
 * data on PhotosynQ.org
 * by: David M. Kramer
 * created: 2017-05-09 @ 18:15:27
 */

// Note: the following variables are for fitting the ECS decay curves when the 
// procedure written by Kevin and Greg fails (which is often). 
// The simple procedure takes the relative changes in the amplitues of the curves
// during the first two and the last two points and compares this ratio to 
// values from simulated curves. It then calculates the ECSt from the gH+ and
// the change in amplitude at the final point. 

// Define the output object here
var output = {}; //dictionary to hold results

// Define the protocol set numbers for each data set

var calib_set=9;

// ECS DIRK trace definitions
var beginning_of_ECS=100; //Note the 100 pulses 
var length_of_ECS_subtrace=220; //The length of each subtrace. 
var number_of_ECS_subtraces=6; // number of ECS subtraces. There are currently 6 of them.
var length_of_ECS_baseline=100;  //there are 100 points in the baseline before the DIRK
var length_of_ECS_all_subtraces=beginning_of_ECS+(number_of_ECS_subtraces*length_of_ECS_subtrace);  
var ECS_averaged_trace=json.set[3].data_raw.slice(beginning_of_ECS,320);  
var begining_of_subtrace_for_linear_fit=75;
var end_of_subtrace_for_linear_fit=145;

var P700_begining_of_subtrace_for_linear_fit=55;
var P700_end_of_subtrace_for_linear_fit=170;

var beginning_of_LEFD=1421; //Note the 100 pulses 
var length_of_LEFd_subtrace=220; //The length of each subtrace. 
var length_of_LEFd_baseline=100;  //there are 100 points in the baseline before the DIRK
var LEFd_trace=json.set[3].data_raw.slice(beginning_of_LEFD,beginning_of_LEFD+length_of_LEFd_subtrace);  

// P700 DIRK trace definitions
var beginning_of_P700_DIRK=100; //Note the 100 pulses 
var length_of_P700_DIRK_subtrace=220; //The length of each subtrace. 
var number_of_P700_DIRK_subtraces=6; // number of ECS subtraces. There are currently 6 of them.
var length_of_P700_DIRK_baseline=100;  //there are 100 points in the baseline before the DIRK
var length_of_P700_DIRK_all_subtraces=beginning_of_P700_DIRK+(number_of_P700_DIRK_subtraces*length_of_P700_DIRK_subtrace);  
var P700_DIRK_averaged_trace=json.set[3].data_raw.slice(beginning_of_P700_DIRK,320);  

// PAM fluorescence traces
//DEFINE THE STARTING AND ENDING POINTS TO USE FOR THE VARIOUS 
//PARAMETERS USED IN THE CALCULATIONS

var Fs_begin=1; //the first point to use for Fs 
var Fs_end =4; //the end point to use  for Fs

//The Fv/FM' results will be calculated using hte Avenson technique, wherein
//a series of different saturation pulse intensities are used and the 
//true saturation point is inferred by extrapolation to infinite
//intensities.

//The first FM value (Fm_1) is obtained with the highest intensity
var Fm_1_begin =101; //the first point to use for the FIRST Fmp
var Fm_1_end =130; //the end point to use  for the FIRST Fmp
var Fm_2_begin =131; //the first point to use for the FIRST Fmp
var Fm_2_end =145; //the end point to use  for the FIRST Fmp
var Fm_3_begin =146; //the first point to use for the FIRST Fmp
var Fm_3_end =160; //the end point to use  for the FIRST Fmp
var Fm_4_begin =161; //the first point to use for the FIRST Fmp
var Fm_4_end =175; //the end point to use  for the FIRST Fmp
var Fm_5_begin =176; //the first point to use for the FIRST Fmp
var Fm_5_end =190; //the end point to use  for the FIRST Fmp

//START AND STOP FOR F0'
var FoPrime_begin =191; //the first point to use for Fs 
var FoPrime_end =205; //the end point to use  for Fs

//SET THE INVERSE INTENSITIES FOR THE AVENSON INTENSITY RAMP
var inverse_intensity = [1/8000,1/7000,1/6000,1/5000];

output.pump="none";

//PSI saturation pulse parameters:
//Calculation of the DIRK delta_A_ECS 
  //output.data_raw_ECS_DIRK = json.set[3].data_raw.slice(0,1420);

var PSI_ss_beg=1; //beginning of the trace for P700 steady-state
var PSI_ss_end=18; //end of the trace for P700 steady-state
var PSI_sat1_beg=25; //beginning of the trace for P700 first saturation pulse
var PSI_sat1_end=170; //end of the trace for P700 first saturation pulse
var PSI_dark_beg=195; //beginning of the trace for P700 steady-state
var PSI_dark_end=200; //end of the trace for P700 steady-state
var PSI_sat2_beg=220; //beginning of the trace for P700 second saturation pulse
var PSI_sat2_end=270; //end of the trace for P700 second saturation pulse


// Start of analyses sections

//ECS trace analysis:

var i, j;
  for (i = 1; i < number_of_ECS_subtraces; i++) {
      var temp=json.set[3].data_raw.slice(i*length_of_ECS_subtrace+beginning_of_ECS,
                                         (i+1)*length_of_ECS_subtrace+beginning_of_ECS);
		for (j = 0; j < length_of_ECS_subtrace; j++) {
    		ECS_averaged_trace[j]=ECS_averaged_trace[j]+temp[j];
    }}

//make up a time axis. the assumption is that all the points are equally spaced in time.
   var fake_time_axis = [];
	for (var i = 1; i <= length_of_ECS_subtrace; i++) {
      ii=i.toFixed(4);
   	fake_time_axis.push(i);
  }
  //Find the best fit line to the baseline points.
  	var reg=MathLINREG(fake_time_axis.slice(begining_of_subtrace_for_linear_fit, end_of_subtrace_for_linear_fit),ECS_averaged_trace.slice(begining_of_subtrace_for_linear_fit, end_of_subtrace_for_linear_fit));
  
  //Generate a line based on the baseline linear regression over the range of values for the full trace.   
  var baseline_offset=[];
  for (j = 0; j < length_of_ECS_subtrace; j++) {
    	jj=j.toFixed(4);
		baseline_offset.push(jj*MathROUND(reg.m, 3) + MathROUND(reg.b));
  }
  
  //output.baseline_offset=baseline_offset;
   //Calculate the deltaI/I0 and convertto approximate delta_A 
	for (j = 0; j < length_of_ECS_subtrace; j++) {
    var rat= ECS_averaged_trace[j]/baseline_offset[j];
	ECS_averaged_trace[j]=-1*MathLOG(rat); //((rat-1)/-2.3);
    }
ECS_averaged_trace[120]=ECS_averaged_trace[119]; //eliminate the spike artifact at end of trace
output.ECS_averaged_trace=ECS_averaged_trace.slice(80,150);

var begin_trace_index=100;
var end_trace_index=120;
var number_of_points_to_fit=end_trace_index-begin_trace_index;
var expData=ECS_averaged_trace.slice(begin_trace_index,end_trace_index);

// Here I assume that the time difference between points was 

var time_per_point=1.5; //entger the delta time between points (only work with constant delta time) 

// Obtain best fit results for ECS decay using non-linear least squares fitting
var tdata=[];
for (i in expData) {
	tdata.push([i*time_per_point, expData[i]]);
}

var a = 1;
var b = 1;
var c = 1;
try{
	var fit = NonLinearRegression(tdata,{
	   equation: 'b + a * e(- x / c)',
	   initial: [ a, b, c ]
	});

	a= fit.parameters[0].value;
	b= fit.parameters[1].value;
	c = fit.parameters[2].value;

	var outdata=[];
	for (i in expData) {
		outdata.push(b + a*Math.exp(-1*i/c));
	}

	output.fitinput=expData;
	output.outdata=outdata;

	// Available for all parameters
	//output.p1_name = fit.parameters[0].name;
	output.ECSt_mAU = MathROUND(fit.parameters[0].value, 5);
	//output.offset = fit.parameters[1].value;
	output.tECS = MathROUND(0.001*fit.parameters[2].value, 4);
	output.gHplus = MathROUND(1000/fit.parameters[2].value, 3);
	var vHplus = output.ECSt_mAU *  output.gHplus;
	output.vHplus = MathROUND(vHplus, 3);

}
catch(e){}


//***********other possible parameters 
//output.p1_sd = fit.parameters[0].sd_error;
//output.p1_p = fit.parameters[0].p;
//output.iterations = fit.iterations;
// Some more info as text
//output.ParameterEstimates = fit.ParameterEstimates;
//output.CovarianceMatrix = fit.CovarianceMatrix;
//output.expFitArray=expFitArray;
//expReg = MathEXPINVREG(expFitArray);
//var ECS_lifetime_ms=expReg.lifetime; //the fitting procedure returns -99999 if it fails to find converge


//Calculaiton of the DIRK delta_P850 

  //output.data_raw_P700_DIRK = json.set[4].data_raw.slice(0,1420);
  //Get sum of all subtraces
  var i, j;
  for (i = 1; i < number_of_P700_DIRK_subtraces; i++) {
      var temp=json.set[4].data_raw.slice(i*length_of_P700_DIRK_subtrace+beginning_of_P700_DIRK,
                                         (i+1)*length_of_P700_DIRK_subtrace+beginning_of_P700_DIRK);
      
		for (j = 0; j < length_of_P700_DIRK_subtrace; j++) {
    		P700_DIRK_averaged_trace[j]=P700_DIRK_averaged_trace[j]+temp[j];
    }
  }

  //make up a time axis. the assumption is that all the points are equally spaced in time.
   var fake_time_axis = [];
	for (var i = 1; i <= length_of_P700_DIRK_subtrace; i++) {
      ii=i.toFixed(4);
   	fake_time_axis.push(i);
  }
  //Find the best fit line to the baseline points.
  	var reg=MathLINREG(fake_time_axis,P700_DIRK_averaged_trace);
  
  //Generate a line based on the baseline linear regression over the range of values for the full trace.   
  var baseline_offset=[];
  for (j = 0; j < length_of_P700_DIRK_subtrace; j++) {
    	jj=j.toFixed(4);
		baseline_offset.push(jj*MathROUND(reg.m, 3) + MathROUND(reg.b));
  }
  
  //output.baseline_offset=baseline_offset;
   //Calculate the deltaI/I0 and convertto approximate delta_A 
	for (j = 0; j < length_of_P700_DIRK_subtrace; j++) {
    var rat= P700_DIRK_averaged_trace[j]/baseline_offset[j];
      
	P700_DIRK_averaged_trace[j]=-1*MathLOG(rat); //((rat-1)/-2.3);
      //test=Math.LN10(rat);
    }
  //replace the artifactual data at positon 120 with the prior value
  //the point has no real valye, so this is just to better visualize the results

P700_DIRK_averaged_trace[120]=P700_DIRK_averaged_trace[119]; 
output.P700_DIRK_averaged_trace=P700_DIRK_averaged_trace.slice(P700_begining_of_subtrace_for_linear_fit, P700_end_of_subtrace_for_linear_fit);

var begin_trace_index=100;
var end_trace_index=120;
var number_of_points_to_fit=end_trace_index-begin_trace_index;
var P700expData=P700_DIRK_averaged_trace.slice(begin_trace_index,end_trace_index);

var P700_time_per_point=1.5; //entger the delta time between points (only work with constant delta time) 

// Obtain best fit results for ECS decay using non-linear least squares fitting
var P700tdata=[];
for (i in expData) {
	P700tdata.push([i*time_per_point, P700expData[i]]);
}

var a = 1;
var b = 1;
var c = 1;

try{
	var fit = NonLinearRegression(P700tdata,{
	   equation: 'b + a * e(- x / c)',
	   initial: [ a, b, c ]
	});

	a= fit.parameters[0].value;
	b= fit.parameters[1].value;
	c = fit.parameters[2].value;

	var P700_outdata=[];
	for (i in expData) {
		P700_outdata.push(b + a*Math.exp(-1*i/c));
	}

	output.P700_fitinput=P700_outdata;
	output.P700_outdata=outdata;

	// Available for all parameters
	output.P700_DIRK_ampl = MathROUND(a, 5);

	output.tP700 = MathROUND(0.001*c, 4);
	output.kP700 = MathROUND(1000/c, 4);
	var v_initial_P700 = output.P700_DIRK_ampl *  output.kP700;
	output.v_initial_P700 = MathROUND(v_initial_P700, 7);
}
catch(e){}

// Display the DIRKf results and calculate LEFd

  output.LEFd_trace = LEFd_trace;

  // Display of PAM result and calculation of the fluorescence parameters
//************************************************************************************************
  //Calculate the PAM fluorescence paramters
  
  output.data_raw_PAM = json.set[5].data_raw.slice(0,310);
  data=json.set[5].data_raw.slice(0,310);
  
// Set our Apparent FmPrime, 3 FmPrime steps, and Fs to calculate both traditional fv/fm and new Multi-phase flash fv/fm
//----------------------------

//get the values for representative Fs 
var baseline=0; //for the time being, do not use baseline

//GET THE VALUES FOR Fs
var Fs = MathMEAN(data.slice(Fs_begin,Fs_end)) - baseline; // take only the first 4 values in the Fs range, excluding the very first
var Fs_std = MathSTDEV(data.slice(Fs_begin,Fs_end)); // create standard deviation for this value for error checking
//output.Fs=Fs;

//GET THE VALUES FOR THE 5 Fm' ILLUMINATION CONDITIONS

var sat_vals = data.slice(Fm_1_begin,Fm_1_end).sort();  // sort the saturating light values from low to high
var AFmP = MathMEAN(sat_vals.slice(2,20)) - baseline; // take the 18 largest values and average them
var AFmP_std = MathSTDEV(sat_vals); // create standard deviation for this value for error checking
//output.AFmP=AFmP;
  
sat_vals = data.slice(Fm_5_begin,Fm_5_end).sort();  // sort the saturating light values from low to high
var FmP_end = MathMEAN(sat_vals.slice(2,23)) - baseline; // take the 21 largest values and average them
var FmP_end_std = MathSTDEV(sat_vals); // create standard deviation for this value for error checking
//output.FmP_end=FmP_end;
  
sat_vals = data.slice(Fm_2_begin,Fm_2_end).sort();  // sort the saturating light values from low to high
var FmP_step1 = MathMEAN(sat_vals.slice(2,6)) - baseline; // take the 4 largest values and average them
var FmP_step1_std = MathSTDEV(sat_vals); // create standard deviation for this value for error checking
//output.FmP_step1=FmP_step1;
  
sat_vals = data.slice(Fm_3_begin,Fm_3_end).sort();  // sort the saturating light values from low to high
var FmP_step2 = MathMEAN(sat_vals.slice(2,6)) - baseline; // take the 4 largest values and average them
var FmP_step2_std = MathSTDEV(sat_vals); // create standard deviation for this value for error checking
//output.FmP_step2=FmP_step2;
  
sat_vals = data.slice(Fm_4_begin,Fm_4_end).sort();  // sort the saturating light values from low to high
var FmP_step3 = MathMEAN(sat_vals.slice(2,6)) - baseline; // take the 4 largest values and average them
var FmP_step3_std = MathSTDEV(sat_vals); // create standard deviation for this value for error checking
//output.FmP_ste32=FmP_step3;
  
// Calculations for F0'
// ----------------------------
var FoPrime_values =   data.slice(FoPrime_begin,FoPrime_end).sort();
  
//var FoPrime = MathMEAN(FoPrime_values.slice(5,10)) - baseline;
var FoPrime = MathMIN(FoPrime_values);
var FoPrime_std = MathSTDEV(FoPrime_values); // create standard deviation for this value for error checking

//output.FoPrime_values=FoPrime_values;
  
// Calculations for corrected FmPrime using multi-phase flash
// ----------------------------
var reg = MathLINREG(inverse_intensity, [AFmP,FmP_step1,FmP_step2,FmP_step3]);

// Calculate Phi2 w/ and w/out multi-phase flash
// ----------------------------
var fvfm_noMPF = (AFmP-Fs)/AFmP;
var fvfm_MPF = (reg.b-Fs)/reg.b;


// Calculate NPQt, PhiNPQ, PhiNO, qL w/ and w/out multi-phase flash
// ----------------------------
var npqt_MPF = (4.88 / ((reg.b / FoPrime) -1) )-1;
var npqt_noMPF = (4.88 / ((AFmP / FoPrime) -1) )-1;
var qL_MPF = ((reg.b - Fs)*FoPrime)/((reg.b-FoPrime)*Fs);
var qL_noMPF = ((AFmP - Fs)*FoPrime)/((AFmP-FoPrime)*Fs);
var PhiNO_MPF = 1/(npqt_MPF + 1 + qL_MPF*4.88); //based on equation 52 in Kramer et al., 2004 PRES
var PhiNO_noMPF = 1/(npqt_noMPF + 1 + qL_noMPF*4.88); //based on equation 52 in Kramer et al., 2004 PRES
var PhiNPQ_MPF = 1-fvfm_MPF-PhiNO_MPF; //based on equation 53 in Kramer et al., 2004 PRES 
var PhiNPQ_noMPF = 1-fvfm_noMPF-PhiNO_noMPF; //based on equation 53 in Kramer et al., 2004 PRES 
var qP_MPF = (reg.b - Fs)/(reg.b - FoPrime);
var qP_noMPF = (FmPrime - Fs)/(FmPrime - FoPrime);
var FvP_FmP_MPF = (reg.b-FoPrime)/reg.b;
var FvP_FmP_noMPF = (AFmP-FoPrime)/AFmP;

// Create the variables to be printed (assume to use the MPF values unless there is a good reason not to)
// ----------------------------
var fvfm = fvfm_MPF;
var npqt = npqt_MPF;
var PhiNO = PhiNO_MPF;
var PhiNPQ = PhiNPQ_MPF;
var qL = qL_MPF;
var FmPrime = reg.b;
var qP = qP_MPF;
var FvP_FmP = FvP_FmP_MPF;
/****************OUTPUT VALUES FROM MACRO *******************/

// if any of the flag conditions are true, then create the 'flag' object.  Otherwise, do not create the flag object.
// for now since flag system isn't fully implemented, also create as separate objects so they will be displayed
// ----------------------------

// If multi-phase flash steps are flat or positive slope, then just use the normal Phi2, NPQt, PhiNPQ, PhiNO... etc.
// If Phi2 or NPQt is less than zero, make zero and give user warning.  If Phi2 is higher than .85, give user danger flag.
// ----------------------------
if (reg.m > 0) {
  fvfm = fvfm_noMPF;
  npqt = npqt_noMPF;
  PhiNO = PhiNO_noMPF;
  PhiNPQ = PhiNPQ_noMPF;
  qL = qL_noMPF;
  FmPrime = AFmP;
  qP = qP_noMPF;
  FvP_FmP = FvP_FmP_noMPF;
  
  if (fvfm <= 0) {
    output.Phi2= 0;
  //  	output.flag.warning.push("Phi2 is negative (should be positive).  It has been set to zero, but check raw trace and consider excluding this point.  To see original negative value, see Phi2_noMPF variable");
    output["warning 2"] = "Phi2 is negative (should be positive).  It has been set to zero, but check raw trace and consider excluding this point.  To see original negative value, see Phi2_noMPF variable";
  }
  if (fvfm >=0.85) {
//  	output.flag.danger.push("Phi2 above the normal range (0 - 0.85).  Please check the raw trace and consider excluding this point.");
	output["danger 5"] = "Phi2 above the normal range (0 - 0.85).  Please check the raw trace and consider excluding this point.";
  }
  else {
	  outputPhi2 		= MathROUND(fvfm,3);
  }
  
  if (npqt <= 0) {
    output.NPQt= 0;
//  	output.flag.warning.push("NPQt is negative (should be positive)!  It has been set to zero, but check raw trace and consider excluding this point.  To see original negative NPQt value, see NPQt_noMPF variable");
	output["warning 1"] = "NPQt is negative (should be positive).  It has been set to zero, but check raw trace and consider excluding this point.  To see original negative value, see NPQt_noMPF variable";
  }
  else {
	  output.NPQt= MathROUND(npqt,3);
  }
	  output.qL= MathROUND(qL,3);
	  output.PhiNPQ= MathROUND(PhiNPQ,3);
	  output.PhiNO= MathROUND(PhiNO,3);
	  output.FvP_over_FmP = MathROUND(FvP_FmP,3);
	  outputqP = MathROUND(qP,3);
}

// Otherwise, use the multi-phase flash calculation for Phi2, NPQt, PhiNPQ, PhiNO... etc.
// If Phi2 or NPQt is less than zero, make zero and give user warning.  If Phi2 is higher than .85, give user danger flag.
// ----------------------------
else {
  if (fvfm <= 0) {
    output.Phi2 = 0;
//  	output.flag.warning.push("Phi2 is negative (should be positive).  It has been set to zero, but check raw trace and consider excluding this point.  To see original negative value, see Phi2_MPF variable");
	output["warning 2"] = "Phi2 is negative (should be positive).  It has been set to zero, but check raw trace and consider excluding this point.  To see original negative value, see Phi2_MPF variable";
  }
  if (fvfm >=0.85) {
//  	output.flag.danger.push("Phi2 above the normal range (0 - 0.85).  Please check the raw trace and consider excluding this point.");
	output["danger 5"] = "Phi2 above the normal range (0 - 0.85).  Please check the raw trace and consider excluding this point.";
  }
  else {
  	output.Phi2= MathROUND(fvfm,3);
  }
  if (npqt <= 0) {
    output.NPQt= 0;
//  	output.flag.warning.push("NPQt is negative (should be positive)!  It has been set to zero, but check raw trace and consider excluding this point.  To see original negative NPQt value, see NPQt_MPF variable");
	output["warning 3"] = "NPQt value is negative (should be positive).  It has been set to zero, but check raw trace and consider excluding this point.  To see original negative value, see NPQt_MPF variable";
  }
  else {
	  output.NPQt= MathROUND(npqt,3);
  }
	  output.qL= MathROUND(qL,3);
	  output.PhiNPQ = MathROUND(PhiNPQ,3);
	  output.PhiNO= MathROUND(PhiNO,3);
	  output.FvP_over_FmP= MathROUND(FvP_FmP,3);
	  output.qP= MathROUND(qP,3);
}

// only display LEF if there is a light intensity measurement > 0 
// ----------------------------
if (typeof json.light_intensity != "undefined" && json.light_intensity > 0) {
	output.LEF= MathROUND((fvfm  * 0.45 * json.light_intensity),3);
}

// Calculate Standard Deviation for Warning or Danger flags (out of bounds measurement)
// ----------------------------

if (Fs_std > 100	) {
//  	output.flag.danger.push("noisy Fs");
	output["danger 1"] = "noisy Fs";
}
if (AFmP_std > 200) {
//  	output.flag.danger.push("noisy FmPrime");
	output["danger 2"] = "noisy FmPrime";
}
if (FmP_step1_std > 60 | FmP_step2_std > 60 | FmP_step3_std > 60 | FmP_end_std > 200) {
//  	output.flag.danger.push("noisy multi-phase flash steps");
	output["danger 3"] = "noisy  multi-phase flash steps";
}
if (FoPrime_std > 150) {
//  	output.flag.danger.push("noisy FoPrime");
	output["danger 4"] = "noisy FoPrime";
}
  
 //ANALYZE THE PHI-PSI DATA  

var ctrace=json.set[5].data_raw;
var PSI_trace_beg=310;
var PSI_trace_end=615;
var PSI_trace_length=PSI_trace_end-PSI_trace_beg;
var PSI_data=json.set[5].data_raw.slice(PSI_trace_beg,PSI_trace_end);
var PSI_dark_raw=MathMEAN(PSI_data.slice(PSI_dark_beg,PSI_dark_end));
var PSI_data_absorbance=[];

for (var i=0; i<PSI_trace_length; i++){
  	  PSI_data_absorbance.push(MathLOG(PSI_dark_raw/PSI_data[i]));
  }
  var PSI_dark=MathMEAN(PSI_data_absorbance.slice(PSI_dark_beg,PSI_dark_end));
  var PSI_ss=MathMEAN(PSI_data_absorbance.slice(PSI_ss_beg,PSI_ss_end));
  var PSI_sat1=MathMEAN(PSI_data_absorbance.slice(PSI_sat1_beg,PSI_sat1_end));
  var PSI_sat2=MathMEAN(PSI_data_absorbance.slice(PSI_sat2_beg,PSI_sat2_end));
  var PSI_ss=1000*MathMEAN(PSI_data_absorbance.slice(PSI_ss_beg,PSI_ss_end));
  var PSI_sat1_vals = PSI_data_absorbance.slice(PSI_sat1_beg,PSI_sat1_end).sort();  // sort the saturating light values from low to high
  var length_of_sat1=PSI_sat1_end-PSI_sat1_beg;
  var top_20_percent=(length_of_sat1*0.8);
  var PSI_sat1 = 1000*MathMEAN(PSI_sat1_vals.slice(top_20_percent,length_of_sat1)); // take the top 20% largest values and average them
  var PSI_sat2_vals = PSI_data_absorbance.slice(PSI_sat2_beg,PSI_sat2_end).sort();  // sort the saturating light values from low to high
  var length_of_sat2=PSI_sat2_end-PSI_sat2_beg;
  var top_20_percent=(length_of_sat2*0.8);
  var PSI_sat2 = 1000*MathMEAN(PSI_sat2_vals.slice(top_20_percent,length_of_sat2)); // take the top 20% largest values and average them
  var PSI_ox=PSI_ss/PSI_sat2;
  var PSI_act=PSI_sat2;
  var PSI_open=(PSI_sat1-PSI_ss)/PSI_sat2;
  var PSI_or=1-PSI_sat1/PSI_sat2;
  output.PSI_data_absorbance=PSI_data_absorbance;

  output.PSI_act=MathROUND(PSI_act, 3);
  output.PSI_open =MathROUND(PSI_open, 3);
  output.PSI_or =MathROUND(PSI_or, 3);
  output.PSI_ox =MathROUND(PSI_ox, 3);
    //output.PSI_dark=PSI_dark;
  //output.PSI_ss = PSI_ss; //MathROUND(PSI_ss, 3);
  //output.PSI_sat1 =MathROUND(PSI_sat1, 3);
  //output.PSI_sat2 =MathROUND(PSI_sat2, 3);

	//output.data_raw_PSI =PSI_data;
   //output.PSI_dark_beg=PSI_dark_beg;
   //output.PSI_dark_end=PSI_dark_end;

// Humidity changes
    var humidity_kinetics=[json.set[1].humidity,json.set[2].humidity,
                        json.set[3].humidity,json.set[4].humidity,
                        json.set[5].humidity,json.set[6].humidity,
                        json.set[7].humidity,json.set[8].humidity
                       ];
  	output.humidity_kinetics=humidity_kinetics;
  
   // changes in leaf contactless_temp
  
    var air_temp_kinetics=[json.set[1].temperature,
                                json.set[2].temperature, 
                                json.set[3].temperature, 
                                json.set[4].temperature, 
                                json.set[5].temperature, 
                                json.set[6].temperature, 
                                json.set[7].temperature, 
                       ];
    
     var contactless_temp_kinetics=[json.set[1].contactless_temp,
                                json.set[2].contactless_temp,
                                json.set[3].contactless_temp,
                                json.set[4].contactless_temp,
                                json.set[5].contactless_temp,
                                json.set[6].contactless_temp,
                                json.set[7].contactless_temp
                       ];
  
  output.air_temp_kinetics=air_temp_kinetics;
  
  output.contactless_temp_kinetics=contactless_temp_kinetics;

 var light_intensity=json.set[3].light_intensity;
  output.light_intensity= light_intensity;                       
  var ambient_RH=json.set[1].humidity;
  output.ambient_RH=MathROUND(ambient_RH,3);
  var ambient_temperature=json.set[1].temperature;
  output.ambient_temperature=ambient_temperature;
  var leaf_RH=json.set[7].humidity;
  output.leaf_RH=MathROUND(leaf_RH, 2);
  leaf_temperature = json.set[3].contactless_temp;
  output.leaf_temperature=leaf_temperature;
  var leaf_air_difference_temperature = leaf_temperature-ambient_temperature;
  output.leaf_air_difference_temperature=MathROUND(leaf_air_difference_temperature,3);

///////////////////////////////

// CALCULATIONS FOR ABSORBANCE / SPAD PORTION OF THE TRACE
//----------------------------

var abs_starts = 0; // when does the Phi2 measurement start
var lights = [1,2,3,4,6,8,9,10];// define the lights to have absorbance measured
var wavelengths = ["530","650","605","420","940","850","730","880"];// define the lights to have absorbance measured
var raw_at_blank1 = [0,0,0,0,0,0,0,0];
var raw_at_blank2 = [0,0,0,0,0,0,0,0];
var raw_at_blank3 = [0,0,0,0,0,0,0,0];
var abs_at_blank1 = [0,0,0,0,0,0,0,0];
var abs_at_blank2 = [0,0,0,0,0,0,0,0];
var abs_at_blank3 = [0,0,0,0,0,0,0,0];
var trans_at_blank1 = [0,0,0,0,0,0,0,0];
var trans_at_blank2 = [0,0,0,0,0,0,0,0];
var trans_at_blank3 = [0,0,0,0,0,0,0,0];
var spad_at_blank1 = [0,0,0,0,0,0,0,0];
var spad_at_blank2 = [0,0,0,0,0,0,0,0];
var spad_at_blank3 = [0,0,0,0,0,0,0,0];
var minolta_spad1 = 0;
var minolta_spad2 = 0;
var minolta_spad3 = 0;
var minolta_spad = 0;
var minolta_spad_averages = 0;
var choose = 0;
var light;
var wavelengthString;
var pulses = 10;// number of pulses in a cycle

/* // this is what the recall object looks like --> 
"recall":{"colorcal_blank1[1]":0.000000,"colorcal_blank1[2]":0.000000,"colorcal_blank1[3]":0.000000,"colorcal_blank1[4]":0.00000
0,"colorcal_blank1[6]":0.000000,"colorcal_blank1[8]":0.000000,"colorcal_blank1[9]":0.000000,"colorcal_blank1[10]":0.000000,"colo
rcal_blank2[1]":0.000000,"colorcal_blank2[2]":0.000000,"colorcal_blank2[3]":0.000000,"colorcal_blank2[4]":0.000000,"colorcal_bla
nk2[6]":0.000000,"colorcal_blank2[8]":0.000000,"colorcal_blank2[9]":0.000000,"colorcal_blank2[10]":0.000000,"colorcal_blank3[1]"
:0.000000,"colorcal_blank3[2]":0.000000,"colorcal_blank3[3]":0.000000,"colorcal_blank3[4]":0.000000,"colorcal_blank3[6]":0.00000
0,"colorcal_blank3[8]":0.000000,"colorcal_blank3[9]":0.000000,"colorcal_blank3[10]":0.000000},
*/

/*
** Loop through the lights.  If it's zero, skip it.  If it's not in the acceptable range (>500 but <65534 (max)) skip it.
** Then choose the first blank (starting with 1 moving to 3) which fulfills this criteria
** Otherwise, calculate absorbance and transmittance and a 'spad'-like value using LED 6 (940 on clamp) to calibrate thickness
*/
//----------------------------
//output.absLength = json.set[9].data_raw.length;
for (var i = 0;i<lights.length;i++) { // loop through and save one averaged 'point' for each of the cycles
 light = lights[i];
 wavelengthString = wavelengths[i];
 var value1 = MathMEDIAN(json.set[9].data_raw.slice((abs_starts + i*pulses+2),(abs_starts + i*pulses+8)));
 var value2 = MathMEDIAN(json.set[9].data_raw.slice((abs_starts + i*pulses+82),(abs_starts + i*pulses+88)));
 var value3 = MathMEDIAN(json.set[9].data_raw.slice((abs_starts + i*pulses+162),(abs_starts + i*pulses+168)));
 raw_at_blank1[light] = value1;
 raw_at_blank2[light] = value2;
 raw_at_blank3[light] = value3;
 if (json.set[9].recall["colorcal_blank1["+light+"]"] !== 0 && raw_at_blank1[light] > 500 && raw_at_blank1[light] < 65534) {
   abs_at_blank1[light] = MathROUND(-1*MathLOG(raw_at_blank1[light]/json.set[9].recall["colorcal_blank1["+light+"]"]),3);
//    trans_at_blank1[light] = MathROUND(raw_at_blank1[light]/json.recall["colorcal_blank1["+light+"]"],3);
//	output ["light".concat(light.toString(),"_transmittance")]  = trans_at_blank1[light];
//   output ["absorbance_".concat(wavelengthString)]  = abs_at_blank1[light];
//	output ["light".concat(light.toString(),"_blank1")]  = json.recall["colorcal_blank1["+light+"]"];
 }
}

// so the raw value needs to be >~2000, while the 940 needs to be greater than ~5000, otherwise it's out of range
// the acceptable range is different for each blank (1,2,3) thus a separate if statement for each blank.
// once you hit an intensity which is within the acceptable range, then keep that value and skip the rest

for (var i = 0;i<8;i++) { // loop through and save one averaged SPAD value for each of the cycles.  If we have additional calibration values (like minolta spad) use those and output that value
 light = lights[i];
 wavelengthString = wavelengths[i];
 spad_at_blank2[light] = MathROUND(100*MathLOG((raw_at_blank2[6] / json.set[9].recall["colorcal_blank2[6]"])/(raw_at_blank2[light] / json.set[9].recall["colorcal_blank2["+light+"]"])),2);
 spad_at_blank1[light] = MathROUND(100*MathLOG((raw_at_blank1[6] / json.set[9].recall["colorcal_blank1[6]"])/(raw_at_blank1[light] / json.set[9].recall["colorcal_blank1["+light+"]"])),2);
 spad_at_blank3[light] = MathROUND(100*MathLOG((raw_at_blank3[6] / json.set[9].recall["colorcal_blank3[6]"])/(raw_at_blank3[light] / json.set[9].recall["colorcal_blank3["+light+"]"])),2);
 if (light == 2) { // if it's the red light, the also calculate minolta spad
//   output ["light".concat(light.toString(),"_raw1")]  = raw_at_blank1[light];
//   output ["light6_raw1"]  = raw_at_blank1[6];
 //  output ["spad_raw1"]  = spad_at_blank1[2];
//   output ["light".concat(light.toString(),"_raw2")]  = raw_at_blank2[light];
//   output ["light6_raw2"]  = raw_at_blank2[6];
//   output ["spad_raw2"]  = spad_at_blank2[2];
//   output ["light".concat(light.toString(),"_raw3")]  = raw_at_blank3[light];
//   output ["light6_raw3"]  = raw_at_blank3[6];
 //  output ["spad_raw3"]  = spad_at_blank3[2];
 }
 if (json.set[9].recall["colorcal_blank1["+light+"]"] !== 0 && raw_at_blank1[light] > 30 && raw_at_blank1[light] < 65534 
     && raw_at_blank1[6] > 4500 && raw_at_blank1[6] < 65534) {
   if (light == 2) { // if it's the red light, the also calculate minolta spad
     minolta_spad = (spad_at_blank1[2] - json.set[9].recall["colorcal_intensity1_yint[2]"]) / json.set[9].recall["colorcal_intensity1_slope[2]"];	
     output ["SPAD_".concat(wavelengthString)]  = MathROUND(minolta_spad,2);
     //output ["SPAD_".concat(wavelengthString,"_intensity")]  = 1;
       choose = 1;
     continue;
   }
   else if (light != 6) {
     //output ["SPAD_".concat(wavelengthString)] = MathROUND(spad_at_blank1[light],2);
     //output ["SPAD_".concat(wavelengthString,"_intensity")] = 1;
//      output ["SPAD1_".concat(wavelengthString)] = MathROUND(spad_at_blank1[light],2);
//      output ["SPAD1_".concat(wavelengthString,"_intensity")] = 1;
     continue;
   }
 }
 if (json.set[9].recall["colorcal_blank2["+light+"]"] !== 0 && raw_at_blank2[light] > 500 && raw_at_blank2[light] < 65534 
     && raw_at_blank2[6] > 3000 && raw_at_blank2[6] < 65534) {
   if (light == 2) { // if it's the red light, the also calculate minolta spad
     //minolta_spad = (spad_at_blank2[2] - json.set[9].recall["colorcal_intensity2_yint[2]"]) / json.set[9].recall["colorcal_intensity2_slope[2]"];
     //output ["SPAD_".concat(wavelengthString)] = MathROUND(minolta_spad,2);
     //output ["SPAD_".concat(wavelengthString,"_intensity")] = 2;
       choose = 2;
     continue;
   }
   else if (light != 6) {
//     output ["SPAD_".concat(wavelengthString)] = MathROUND(spad_at_blank2[light],2);
//     output ["SPAD_".concat(wavelengthString,"_intensity")] = 2;
//      output ["SPAD2_".concat(wavelengthString)] = MathROUND(spad_at_blank2[light],2);
//      output ["SPAD2_".concat(wavelengthString,"_intensity")] = 2;
     continue;
   }
 }
 if (json.set[9].recall["colorcal_blank3["+light+"]"] !== 0 && raw_at_blank3[light] > 750 && raw_at_blank3[light] < 65534 
     && raw_at_blank3[6] > 3000 && raw_at_blank3[6] < 65534) {
   if (light == 2) { // if it's the red light, the also calculate minolta spad
     minolta_spad = (spad_at_blank3[2] - json.set[9].recall["colorcal_intensity3_yint[2]"]) / json.set[9].recall["colorcal_intensity3_slope[2]"];
     //output ["SPAD_".concat(wavelengthString)]  = MathROUND(minolta_spad,2);
     //output ["SPAD_".concat(wavelengthString,"_intensity")]  = 3;
     choose = 3;
     continue;
   }
   else if (light != 6) {
//     output ["SPAD_".concat(wavelengthString)] = MathROUND(spad_at_blank3[light],2);
//     output ["SPAD_".concat(wavelengthString,"_intensity")] = 3;
//      output ["SPAD3_".concat(wavelengthString)] = MathROUND(spad_at_blank3[light],2);
//      output ["SPAD3_".concat(wavelengthString,"_intensity")] = 3;
     continue;
   }
 }
}

if (choose === 0) {
 output.SPAD_650 = 0;
 danger("Chlorophyll content SPAD is outside the acceptable range.  The leaf may be too thick, too thin, or have holes in it.", output);
}
else if (minolta_spad <= 2) {
 danger("Chlorophyll Content SPAD is very low.  If leaf is visibly green, ensure leaf completely covers the light guide and retry.  If still too low, consider recalibrating device.", output);
}
else if (minolta_spad >= 100) {
 danger("Chlorophyll Content SPAD is very high.  If this value is associated with a typical leaf, consider recalibrating device.", output);
}

/*
   if (minolta_spad <= 2) {
     info("Chlorophyll Content SPAD is very low.  If leaf is visibly green, ensure leaf completely covers the light guide and retry.  If still too low, consider recalibrating device.", output);
   }
   else if (minolta_spad1 >= 100) {
     info("Chlorophyll Content SPAD is very high.  If this value is associated with a typical leaf, consider recalibrating device.", output);
   }
*/

// calculate chlorophyll content SPAD values with Minolta SPAD calibration -->

/*
if (choose == 0) {
   output ["SPAD"]  = 0;
   output ["SPAD intensity"]  = 0;
 	
	danger("Chlorophyll content SPAD is outside the acceptable range.  The leaf may be too thick, too thin, or have holes in it.", output);
}
else if (choose == 1) {
   output ["SPAD"]  = MathROUND(minolta_spad1,2);
   output ["SPAD intensity"]  = 1;
   if (minolta_spad1 <= 2) {
     info("Chlorophyll Content SPAD is very low.  If leaf is visibly green, ensure leaf completely covers the light guide and retry.  If still too low, consider recalibrating device.", output);
   }
   else if (minolta_spad1 >= 100) {
     info("Chlorophyll Content SPAD is very high.  If this value is associated with a typical leaf, consider recalibrating device.", output);
   }
}
else if (choose == 2) {
   output ["SPAD"]  = MathROUND(minolta_spad2,2);
   output ["SPAD intensity"]  = 2;
	if (minolta_spad2 <= 2) {
     info("Chlorophyll Content SPAD is very low.  If leaf is visibly green, ensure leaf completely covers the light guide and retry.  If still too low, consider recalibrating device.", output);
   }
   else if (minolta_spad2 >= 100) {
     info("Chlorophyll Content SPAD is very high.  If this value is associated with a typical leaf, consider recalibrating device.", output);
   }
}
else if (choose == 3) {
   output ["SPAD"]  = MathROUND(minolta_spad3,2);
   output ["SPAD intensity"]  = 3;
	if (minolta_spad3 <= 2) {
     info("Chlorophyll Content SPAD is very low.  If leaf is visibly green, ensure leaf completely covers the light guide and retry.  If still too low, consider recalibrating device.", output);
   }
   else if (minolta_spad3 >= 100) {
     info("Chlorophyll Content SPAD is very high.  If this value is associated with a typical leaf, consider recalibrating device.", output);
   }
}
*/

output.thick1=json.set[2].thickness;
output.thick2=json.set[3].thickness;
output.thick3=json.set[5].thickness;
output.thick4=json.set[9].thickness;


// Return data
return output;