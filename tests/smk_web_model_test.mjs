// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import { Updater, sameModel } from '../web/fm1ota.js';
const models=['FM-1','SMK-37 Pro','SMK-37 Elite','MKE-P37'];
let checks=0;
for(const device of models)for(const target of models){
  assert.equal(sameModel('ota-'+device,target),device===target);checks++;
  let writes=0,closed=0;
  const u=Object.create(Updater.prototype);
  u.find=async()=>({id:{model:'ota-'+device,text:'ota-'+device+'_900'},link:{close(){closed++;}}});
  u.write=async()=>{writes++;return {finished:false};};
  if(device===target){assert.equal(await u.resume(new Uint8Array(),null,target+'_900'),false);assert.equal(writes,1);}
  else{await assert.rejects(()=>u.resume(new Uint8Array(),null,target+'_900'),e=>e.code==='model');assert.equal(writes,0);assert.equal(closed,1);}
  checks++;
  u.find=async()=>({id:{model:device,text:device+'_016'},link:{close(){closed++;},send(){throw Error('wrong-model write');}}});
  if(device!==target){await assert.rejects(()=>u.install(new Uint8Array(),target+'_900'),e=>e.code==='model');checks++;}
}
{
  const u=Object.create(Updater.prototype);let writes=0;
  u.find=async()=>({id:{model:'ota-SMK-37 Elite',text:'ota-SMK-37 Elite_900'},link:{close(){}}});
  u.write=async()=>{writes++;};
  await assert.rejects(()=>u.resume(new Uint8Array()),e=>e.code==='model');assert.equal(writes,0);checks++;
}
console.log(`SMK browser model checks: ${checks} passed; no hardware accessed`);
